#include <wx/app.h>
#include <wx/event.h>
#include "SerialDriver.h"
#include "SerialThread.h"

#include <filesystem>

#include "SerialPortEnumerator.h"
#include "Events.h"

#include <iostream>
#include <thread>

#include "MeasurementEvent.h"

enum FrameType { OSD_MODE_20V = 20, OSD_MODE_28V = 28 };

static int8_t hex2bin(unsigned char c) {
    if (c >= '0' && c <= '9') {
        return c - 0x30;
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    return 0;
}

static int16_t hex4_to_uint16(const char *buf) {
    int16_t val = 0 | (hex2bin(buf[0]) << 12) | (hex2bin(buf[1]) << 8) |
                  (hex2bin(buf[2]) << 4) | hex2bin(buf[3]);
    return val;
}

static void hexdump(const char *buffer) {
    bool isEof = false;
    bool isEof2 = false;
    for (int i = 0;; i += 16) {
        printf("%04x:  ", i);
        for (int j = 0; j < 16; j++) {
            if (isEof) {
                printf("%2s ", " ");
            } else {
                printf("%02x ", buffer[i + j] & 0xff);
                if (buffer[i + j] == 0x00) {
                    isEof = true;
                }
            }
        }
        printf("%s", "  ");
        for (int j = 0; j < 16; j++) {
            if (isEof2) {
                printf("%s", " ");
            } else {
                printf("%c ", isprint(buffer[i + j]) ? buffer[i + j] : '.');
                if (buffer[i + j] == 0x00) {
                    isEof2 = true;
                }
            }
        }
        printf("\n");
        if (isEof) {
            break;
        }
    }
}


void SerialThread::updateStatus(const wxString &status) {
    auto *event = new wxThreadEvent(wxEVT_STATUS_UPDATE);
    event->SetString(status);
    if (!this->m_frame) {
        std::cerr << "m_frame is NULL!!!" << std::endl;
        return;
    }
    if (!TestDestroy()) {
        wxQueueEvent(this->m_frame, event);
    }
}

bool SerialThread::measure_loop(const std::string &device) {
    auto port = new ceSerial(device, 9600, 8, 'N', 1);
    if (port->Open() != 0) {
        std::cerr << "Failed to open" << device << std::endl;
        return false;
    }
    char line[100];
    int bytes_read = port->ReadLine(line, 100, 1000);
    if (bytes_read < 0) {
        std::cerr << "Read error" << std::endl;
        return false;
    }

    bytes_read = port->ReadLine(line, 100, 1000);
    if (bytes_read < 0) {
        std::cerr << "Read error or timeout" << std::endl;
        port->Close();
        return false;
    }
    // hexdump(line);
    if (bytes_read != 8 && bytes_read != 9) {
        port->Close();
        return false;
    }
    int frame_type;
    if (bytes_read == 9) {
        frame_type = OSD_MODE_20V;
        if (line[8] == OSD_MODE_28V) {
            frame_type = OSD_MODE_28V;
        } else if (line[8] == OSD_MODE_20V) {
            frame_type = OSD_MODE_20V;
        }
    } else if (bytes_read == 8) {
        frame_type = OSD_MODE_20V;
    } else {
        std::cerr << "Cannot obtain frame type" << std::endl;
        port->Close();
        return false;
    }
    while (true) {
        double voltage_quanta;
        double current_quanta; // with 100mR shunt

        if (TestDestroy()) return false;

        bytes_read = port->ReadLine(line, 100, 250);
        if (bytes_read < 0) {
            std::cerr << "Read error or timeout" << std::endl;
            break;
        }
        // hexdump(line);

        if (frame_type == OSD_MODE_28V) {
            voltage_quanta = 3.125;
            current_quanta = 0.2; // with 50mR shunt
        } else {
            voltage_quanta = 4.0;
            current_quanta = 0.06; // with 100mR shunt
        }
        // printf("len=%d\n", static_cast<int>(strlen(serial_buffer)));
        if (strlen(line) < 8 || strlen(line) > 11) {
            continue;
        }
        int shunt_voltage = hex4_to_uint16(line);
        // printf("Shunt: %d\n", shunt_voltage);

        double bus_voltage = static_cast<double>(hex4_to_uint16(line + 4));
        // if (frame_type == OSD_MODE_20V && (bus_voltage & 0x0001)) {
        //     std::cerr << "bad data?" << std::endl;
        //     break;
        // }

        if (frame_type == OSD_MODE_20V)
            bus_voltage /= 8.0;


        int milliamps = abs((int) (static_cast<double>(shunt_voltage) * current_quanta));
        int millivolts = static_cast<int>(bus_voltage * voltage_quanta);

        if (TestDestroy()) break;
        MeasurementEvent *event;
        event = new MeasurementEvent(millivolts, milliamps);
        wxQueueEvent(this->m_frame, event);
    }
    if (!TestDestroy()) {
        wxQueueEvent(this->m_frame, new MeasurementEvent(0, 0));
    }
    port->Close();
    return true;
}

wxThread::ExitCode SerialThread::Entry() {
    updateStatus("Thread started");

    while (true) {
        auto enumerator = new SerialPortEnumerator();
        if (TestDestroy())
            break;

        for (const auto& port: enumerator->GetPortNames()) {
            updateStatus(wxString::Format("Trying %s",
                                          std::filesystem::path(std::string(port.c_str())).filename().string()
            ));
            this->measure_loop(std::string(port.c_str()));
        }
        delete enumerator;
        updateStatus("Waiting for device");
        wxThread::Sleep(1000);
    }
    return (wxThread::ExitCode) nullptr;
}
