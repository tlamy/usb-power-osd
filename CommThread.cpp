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

#include "JsonParser.h"

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

static void hexdump(const unsigned char *buffer) {
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

void CommThread::parseUSBMeterData(const SimpleBLE::ByteArray& data) {
  std::string json(reinterpret_cast<const char*>(data.data()), data.size());

  // Ensure null termination if needed
  if (json.back() != '\0') {
    json += '\0';
  }

  float voltage = 0;
  float current = 0;
  float power = 0;
  float charge = 0;
  unsigned long timestamp = 0;  // uptime of mcu in ms

  JsonParser parser;
  auto map = parser.parseObject(json);
  if (map.find("voltage") != map.end()) {
    voltage = std::stof(map["voltage"]);
  }
  if (map.find("current") != map.end()) {
    current = std::stof(map["current"]);
  }
  if (map.find("power") != map.end()) {
    power = std::stof(map["power"]);
  }
  if (map.find("energy") != map.end()) {
    charge = std::stof(map["charge"]);
  }
  if (map.find("timestamp") != map.end()) {
    timestamp = std::stoul(map["timestamp"]);
  }

  if (voltage != 0.0 && current != 0.0) {
    wxQueueEvent(m_frame, new MeasurementEvent(voltage, current,power,charge,timestamp));
  }
}

void CommThread::handleReceivedData(const SimpleBLE::ByteArray& data) {
  std::cout << "Received " << data.size() << " bytes: ";

  if (data[0] != '{' || data[data.size() - 1] != '}') {
    std::cout << "Not a valid JSON object: " << std::endl;
      // Print as hex
      for (size_t i = 0; i < data.size(); ++i) {
        printf("%02X ", static_cast<unsigned char>(data[i]));
      }
      std::cout << std::endl;
    return;
  }

  // Parse your specific data format here
  parseUSBMeterData(data);
}

bool CommThread::ble_measure_loop(SimpleBLE::Peripheral peripheralx) {
  try {
    SelectedDevice *currentDevice = this->device.load(); // Atomic read
    if (currentDevice->getType() != DeviceType::BLE) {
      std::cerr << "Wrong device type" << std::endl;
      return false;
    }
    auto adapters = SimpleBLE::Adapter::get_adapters();
    if (adapters.empty())
      throw std::runtime_error("No adapters");

    auto adapter = adapters[0];
    auto results = adapter.scan_get_results(); // Uses cached results!

    SimpleBLE::Peripheral peripheral;
    for (auto &item : results) {
      if (item.address() == currentDevice->getDeviceInfo().ToStdString()) {
        peripheral = item;
        break;
      }
    }
    if (!peripheral.is_connectable() && !peripheral.is_connected())
      throw std::runtime_error("Device not found in scan cache");

    auto target_peripheral = peripheral;

    // Connect to the device
    std::cerr << "Checking connection" << std::endl;
    if (!target_peripheral.is_connected()) {
      std::cerr << "Connecting to BLE device..." << std::endl;
      target_peripheral.connect();
      if (!target_peripheral.is_connected()) {
        std::cerr << "Failed to connect to BLE device" << std::endl;
        return false;
      }
    } else {
      std::cerr << "BLE device already connected." << std::endl;
    }
    std::cerr << "Setup should be here" << std::endl;
    peripheral.notify(SERVICE_UUID, CHARACTERISTIC_UUID,
        [this](SimpleBLE::ByteArray data) {
            handleReceivedData(data);
        });

    std::cout << "Notifications set up successfully!" << std::endl;

  } catch (const std::exception &e) {
    std::cerr << "Exception in BLE measure loop: " << e.what() << std::endl;
    print_stacktrace();
    return false;
  }
}

bool CommThread::serial_measure_loop(const std::string &device) {
  auto port = new serialib();
  char code;
  if ((code = port->openDevice(device.c_str(), 9600)) != 1) {
    std::cerr << "Failed to open " << device << " return code=" << code
              << std::endl;
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
    SelectedDevice *currentDevice = this->device.load(); // Atomic read
    if (currentDevice == nullptr) {
      Sleep(1000);
      continue;
    }
    std::cout << "There is a selected device..." << std::endl;
    if (TestDestroy())
      break;
    if (currentDevice->getType() == DeviceType::BLE) {
      this->ble_measure_loop(currentDevice->getBlePeripheral());
    } else if (currentDevice->getType() == DeviceType::Serial) {
      this->serial_measure_loop(currentDevice->getDeviceInfo().ToStdString());
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
  std::cout << "device.BLEdevice= " << std::hex
            << static_cast<const void *>(&device->getBlePeripheral())
            << std::endl;

  this->device.store(device); // Atomic write
}
