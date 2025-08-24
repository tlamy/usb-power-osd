#include "CommThread.h"
#include "serialib.h"
#include <wx/app.h>
#include <wx/event.h>

#include <filesystem>

#include "Events.h"
#include "SerialPortEnumerator.h"

#include <iostream>
#include <thread>

#include "MeasurementEvent.h"
#include <simpleble/SimpleBLE.h>

#define SERVICE_UUID "01bc9d6f-5b93-41bc-b63f-da5011e34f68"
#define CHARACTERISTIC_UUID "307fc9ab-5438-4e03-83fa-b9fc3d6afde2"

#include <charconv>
#include <cstdlib>
#include <execinfo.h>

static void print_stacktrace() {
  void *array[10];
  size_t size = backtrace(array, 10);
  char **strings = backtrace_symbols(array, size);
    
  std::cerr << "Obtained " << size << " stack frames:\n";
  for (size_t i = 0; i < size; i++) {
    std::cerr << strings[i] << std::endl;
  }
    
  free(strings);
}

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
  const int16_t val =
      0 | (hex2bin(buf[0]) << 12) |
      (hex2bin(buf[1]) << 8) | // NOLINT(*-narrowing-conversions)
      (hex2bin(buf[2]) << 4) | hex2bin(buf[3]);
  return val;
}

void CommThread::updateStatus(const wxString &status) {
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

bool CommThread::ble_measure_loop() {
  try {
    if (device->getType() != DeviceType::BLE) {
      std::cerr << "Wrong device type" << std::endl;
      return false;
    }

      std::cerr << "cpoy peripheral" << std::endl;

    auto target_peripheral = device->getBlePeripheral();

    if (!target_peripheral.is_connected()) {
      std::cerr << "BLE not connected yet" << std::endl;
      target_peripheral.connect();
      if (!target_peripheral.is_connected()) {
        std::cerr << "BLE device not connected" << std::endl;
        return false;
      }
    }
    std::cerr << "cpoy peripheral" << std::endl;

    // Wait a bit for service discovery
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Find the service and characteristic
    bool service_found = false;
    bool characteristic_found = false;

    std::cerr << "scan services" << std::endl;

    for (auto &service : target_peripheral.services()) {
      if (service.uuid() == SERVICE_UUID) {
        std::cerr << "BLE service found on device" << std::endl;
        service_found = true;
        for (auto &characteristic : service.characteristics()) {
          if (characteristic.uuid() == CHARACTERISTIC_UUID) {
            std::cerr << "Found characteristic on device" << std::endl;
            characteristic_found = true;
            break;
          }
        }
        break;
      }
    }

    if (!service_found || !characteristic_found) {
      std::cerr << "Required service or characteristic not found" << std::endl;
      target_peripheral.disconnect();
      return false;
    }

    // ... rest of your measurement loop logic
    return true;
    
  } catch (const std::exception& e) {
    std::cerr << "Exception in BLE measure loop: " << e.what() << std::endl;
    print_stacktrace();
    return false;
  } catch (...) {
    std::cerr << "Unknown exception in BLE measure loop" << std::endl;
    return false;
  }
}

bool CommThread::serial_measure_loop(const std::string &device) {
  auto port = new serialib();
    char code;
    if ((code = port->openDevice(device.c_str(), 9600)) != 1) {
      std::cerr << "Failed to open " << device << " return code=" << code << std::endl;
      delete port;
      return false;
    }

    port->flushReceiver();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    char line[100];
    int bytes_read = port->readString(line, '\n', 100, 5000);
    if (bytes_read < 0) {
      std::cerr << "Read error" << std::endl;
      port->closeDevice();
      delete port;
      return false;
    }

  bytes_read = port->readString(line, '\n', 100, 1000);
  if (bytes_read < 0) {
    std::cerr << "Read error or timeout" << std::endl;
      port->closeDevice();
      delete port;
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

    if (TestDestroy())
      return false;

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

    int milliamps = abs(
        static_cast<int>(static_cast<double>(shunt_voltage) * current_quanta));
    int millivolts = static_cast<int>(bus_voltage * voltage_quanta);

    if (TestDestroy())
      break;
    auto event = new MeasurementEvent(millivolts, milliamps);
    wxQueueEvent(this->m_frame, event);
  }
  if (!TestDestroy()) {
    wxQueueEvent(this->m_frame, new MeasurementEvent(0, 0));
  }
  port->closeDevice();
  return true;
}

wxThread::ExitCode CommThread::Entry() {
  updateStatus("Thread started");

  while (true) {
    if (this->device == nullptr) {
      Sleep(1000);
      continue;
    }
    std::cout << "=== Starting port enumeration ===" << std::endl;
    if (TestDestroy())
      break;
    if (this->device->getType() == DeviceType::BLE) {
      this->ble_measure_loop();
    }

    auto ports = SerialPortEnumerator::GetPortNames(); // Changed to static call

    for (const auto &port : ports) {
      std::cout << "Processing port: " << port.ToStdString() << std::endl;
      updateStatus(wxString::Format(
          "Trying %s", std::filesystem::path(std::string(port.c_str()))
                           .filename()
                           .string()));
      this->serial_measure_loop(std::string(port.c_str()));
    }
    updateStatus("Waiting for device");
    Sleep(1000);
  }
  return nullptr;
}

void CommThread::setDevice(SelectedDevice *device) {
  std::cout << "Set device to " << device->getDisplayName() << std::endl;
  std::cout << "device.BLEdevice= " << std::chars_format(hex(device->getBlePeripheral()) << std::endl;

  this->device = device;
}
