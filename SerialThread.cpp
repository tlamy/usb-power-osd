#include <wx/app.h>
#include <wx/event.h>
#include "serialib.h"
#include "SerialThread.h"

#include <filesystem>

#include "SerialPortEnumerator.h"
#include "Events.h"

#include <iostream>
#include <thread>

#include "MeasurementEvent.h"

enum FrameType { OSD_MODE_20V = 20, OSD_MODE_28V = 28 };

static int8_t hex2bin(const unsigned char c) {
    if (c >= '0' && c <= '9') {
        return c - '0'; // NOLINT(*-narrowing-conversions)
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10; // NOLINT(*-narrowing-conversions)
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10; // NOLINT(*-narrowing-conversions)
    }
    return 0;
}

static int16_t hex4_to_uint16(const char *buf) {
    const int16_t val = 0 | (hex2bin(buf[0]) << 12) | (hex2bin(buf[1]) << 8) | // NOLINT(*-narrowing-conversions)
                        (hex2bin(buf[2]) << 4) | hex2bin(buf[3]);
    return val;
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
    auto port = new serialib();
    char code;
    if ((code = port->openDevice(device.c_str(), 9600)) != 1) {
        std::cerr << "Failed to open" << device << " return code=" << code << std::endl;
        return false;
    }
    char line[100];
    int bytes_read = port->readString(line, '\n', 100, 1000);
    if (bytes_read < 0) {
        std::cerr << "Read error" << std::endl;
        return false;
    }

    bytes_read = port->readString(line, '\n', 100, 1000);
    if (bytes_read < 0) {
        std::cerr << "Read error or timeout" << std::endl;
        port->closeDevice();
        return false;
    }
    // hexdump(line);
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
        port->closeDevice();
        return false;
    }
    while (true) {
        double voltage_quanta;
        double current_quanta; // with 100mR shunt

        if (TestDestroy()) return false;

        bytes_read = port->readString(line, '\n', 100, 250);
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

        auto bus_voltage = static_cast<double>(hex4_to_uint16(line + 4));
        // if (frame_type == OSD_MODE_20V && (bus_voltage & 0x0001)) {
        //     std::cerr << "bad data?" << std::endl;
        //     break;
        // }

        if (frame_type == OSD_MODE_20V)
            bus_voltage /= 8.0;


        int milliamps = abs(static_cast<int>(static_cast<double>(shunt_voltage) * current_quanta));
        int millivolts = static_cast<int>(bus_voltage * voltage_quanta);

        if (TestDestroy()) break;
        auto event = new MeasurementEvent(millivolts, milliamps);
        wxQueueEvent(this->m_frame, event);
    }
    if (!TestDestroy()) {
        wxQueueEvent(this->m_frame, new MeasurementEvent(0, 0));
    }
    port->closeDevice();
    return true;
}

wxThread::ExitCode SerialThread::Entry() {
    updateStatus("Thread started");

    while (true) {
        auto enumerator = new SerialPortEnumerator();
        if (TestDestroy())
            break;

        for (const auto &port: enumerator->GetPortNames()) {
            updateStatus(wxString::Format("Trying %s",
                                          std::filesystem::path(std::string(port.c_str())).filename().string()
            ));
            this->measure_loop(std::string(port.c_str()));
        }
        delete enumerator;
        updateStatus("Waiting for device");
        Sleep(1000);
    }
    return nullptr;
}
