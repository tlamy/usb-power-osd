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

enum FrameType { OSD_MODE_20V, OSD_MODE_28V };

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

static void hexdump(char *buffer) {
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


void SerialThread::updateStatus(wxString status) {
    wxThreadEvent *event = new wxThreadEvent(wxEVT_STATUS_UPDATE);
    event->SetString(status);
    std::cerr << "Sending status update: " << status << std::endl;
    if (!this->m_frame) {
        std::cerr << "m_frame is NULL!!!" << std::endl;
        return;
    }
    wxQueueEvent(this->m_frame, event);
}

int SerialThread::ReadLine(ceSerial *port, char *buffer, int size, int timeout = 1000) {
    int index = 0;
    bool success;
    char c;
    auto start = std::chrono::high_resolution_clock::now();
    do {
        c = port->ReadChar(success);
        if (success) {
            buffer[index++] = c;
        } else {
            std::cerr << "No data available" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        if (index >= size - 1) {
            return -1;
        }
        if (std::chrono::high_resolution_clock::now() - start > std::chrono::milliseconds(timeout)) {
            std::cerr << "Timeout" << std::endl;
            return -1;
        }
    } while (c != '\n');
    buffer[index] = '\0';
    if (index > 0)buffer[index - 1] = '\0';
    return index;
}

bool SerialThread::measure_loop(std::string device) {
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
    std::cerr << "Read: " << line << std::endl;

    bytes_read = port->ReadLine(line, 100, 1000);
    if (bytes_read < 0) {
        std::cerr << "Read error or timeout" << std::endl;
        port->Close();
        return false;
    }
    //std::cerr << "Read: " << line << std::endl;
    hexdump(line);
    if (bytes_read != 10 && bytes_read != 9) {
        std::cerr << "Wrong string length: " << bytes_read << std::endl;
        port->Close();
        return false;
    }
    int frame_type;
    if (bytes_read == 10) {
        frame_type = OSD_MODE_20V;
        if (line[8] == OSD_MODE_28V) {
            frame_type = OSD_MODE_28V;
        } else if (line[8] == OSD_MODE_20V) {
            frame_type = OSD_MODE_20V;
        }
    } else if (bytes_read == 9) {
        frame_type = OSD_MODE_20V;
    } else {
        std::cerr << "Cannot obtain frame type" << std::endl;
        port->Close();
        return false;
    }
    while (true) {
        double voltage_quanta;
        double current_quanta; // with 100mR shunt

        bytes_read = port->ReadLine(line, 100, 1000);
        if (bytes_read < 0) {
            std::cerr << "Read error or timeout" << std::endl;
            break;
        }
        hexdump(line);

        if (frame_type == OSD_MODE_28V) {
            voltage_quanta = 3.125;
            current_quanta = 0.2; // with 50mR shunt
        } else {
            voltage_quanta = 4.0;
            current_quanta = 0.06; // with 100mR shunt
        }
        // printf("len=%d\n", static_cast<int>(strlen(serial_buffer)));
        if (strlen(line) < 8 || strlen(line) > 11) {
            std::cerr << "String wrong length " << strlen(line) << std::endl;
            continue;
        }
        int shunt_voltage = hex4_to_uint16(line);
        // printf("Shunt: %d\n", shunt_voltage);

        int bus_voltage = hex4_to_uint16(line + 4);

        // if (frame_type == OSD_MODE_20V && (bus_voltage & 0x0001)) {
        //     std::cerr << "bad data?" << std::endl;
        //     break;
        // }

        if (frame_type == OSD_MODE_20V)
            bus_voltage = bus_voltage >> 3;

        int milliamps = abs((int) (static_cast<double>(shunt_voltage) * current_quanta));
        int millivolts = (int) (static_cast<double>(bus_voltage) * voltage_quanta);

        MeasurementEvent *event;
        if (millivolts < 1000) {
            event = new MeasurementEvent(millivolts, 0);
        } else {
            event = new MeasurementEvent(millivolts, milliamps);
        }
        wxQueueEvent(this->m_frame, event);
        wxThread::Sleep(50);
    }
    port->Close();
    return true;
}

wxThread::ExitCode SerialThread::Entry() {
    updateStatus("Thread started");


    while (true) {
        auto enumerator = new SerialPortEnumerator();
        if (TestDestroy())
            return (ExitCode) 0;

        wxLogDebug("Searching for serial devices...");
        updateStatus("Searching...");
        p_mode = SEARCH;
        if (p_mode == SEARCH) {
            for (auto port: enumerator->GetPortNames()) {
                std::cerr << "Trying" << port << std::endl;
                updateStatus(wxString::Format("Trying %s",
                                              std::filesystem::path(std::string(port.c_str())).filename().string()
                ));
                this->measure_loop(std::string(port.c_str()));
            }
        }
        delete enumerator;
        updateStatus("...");
        wxLogDebug("Serial device search complete.");

        wxThread::Sleep(5000); // Simulate long operation
    }
    return (wxThread::ExitCode) nullptr;
}
