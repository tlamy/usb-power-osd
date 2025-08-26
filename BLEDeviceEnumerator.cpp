#include "BLEDeviceEnumerator.h"

#include "third-party/SimpleBLE/simpleble/src/backends/common/PeripheralBase.h"

#include <simpleble/SimpleBLE.h>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>

class BLEDeviceEnumerator::Impl {
public:
  Impl() : scanning(false) {
    try {
      auto adapters = SimpleBLE::Adapter::get_adapters();
      if (!adapters.empty()) {
        adapter = std::make_unique<SimpleBLE::Adapter>(adapters[0]);
        std::cout << "BLE Adapter initialized: " << adapter->identifier()
                  << std::endl;
      } else {
        std::cout << "No BLE adapters found" << std::endl;
      }
    } catch (const std::exception &e) {
      std::cerr << "Error initializing BLE adapter: " << e.what() << std::endl;
    }
  }

  ~Impl() { StopScan(); }

  bool StartScan(int timeoutMs) {
    if (!adapter) {
      std::cerr << "No BLE adapter available" << std::endl;
      return false;
    }

    try {
      discoveredDevices.clear();
      scanning = true;

      std::cout << "Setting up BLE scan callback..." << std::endl;
      adapter->set_callback_on_scan_found(
          [this](SimpleBLE::Peripheral peripheral) {
            std::cout << "Found BLE device: " << peripheral.identifier() << " ("
                      << peripheral.address() << ")" << std::endl;
            if (peripheral.identifier().empty()) {
              return;
            }
            // Avoid duplicates
            auto it = std::find_if(
                discoveredDevices.begin(), discoveredDevices.end(),
                [&peripheral](SimpleBLE::Peripheral &existing) {
                  return existing.address() == peripheral.address();
                });

            if (it == discoveredDevices.end()) {
              discoveredDevices.push_back(peripheral);
            } else {
              // // Update existing device info (e.g., RSSI might have changed)
              // it->s = peripheral.rssi();
              // it->connectable = deviceInfo.connectable;
            }
          });

      std::cout << "Starting BLE scan for " << timeoutMs << "ms..."
                << std::endl;

      // Start scanning in a separate thread to avoid blocking
      std::thread scanThread([this, timeoutMs]() {
        try {
          adapter->scan_for(timeoutMs);
        } catch (const std::exception &e) {
          std::cerr << "Error during BLE scan: " << e.what() << std::endl;
        }
        scanning = false;
        std::cout << "BLE scan completed" << std::endl;
      });
      scanThread.detach();

      return true;
    } catch (const std::exception &e) {
      std::cerr << "Failed to start BLE scan: " << e.what() << std::endl;
      scanning = false;
      return false;
    }
  }

  void StopScan() {
    if (adapter && scanning) {
      try {
        adapter->scan_stop();
        std::cout << "BLE scan stopped" << std::endl;
      } catch (const std::exception &e) {
        std::cerr << "Error stopping BLE scan: " << e.what() << std::endl;
      }
    }
    scanning = false;
  }

  bool IsScanning() const { return scanning; }

  std::vector<SimpleBLE::Peripheral> GetDiscoveredDevices() const {
    return discoveredDevices;
  }

  void ClearDiscoveredDevices() { discoveredDevices.clear(); }

  bool IsBluetoothAvailable() const { return adapter != nullptr; }

private:
  std::unique_ptr<SimpleBLE::Adapter> adapter;
  std::vector<SimpleBLE::Peripheral> discoveredDevices;
  std::atomic<bool> scanning; // Make it atomic for thread safety
};

// BLEDeviceEnumerator implementation
BLEDeviceEnumerator::BLEDeviceEnumerator() : pImpl(std::make_unique<Impl>()) {}

BLEDeviceEnumerator::~BLEDeviceEnumerator() = default;

std::vector<SimpleBLE::Peripheral>
BLEDeviceEnumerator::GetBLEDevices(int scanTimeoutMs) {
  BLEDeviceEnumerator enumerator;
  if (enumerator.StartScan(scanTimeoutMs)) {
    // Wait for scan to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(scanTimeoutMs + 100));
    return enumerator.GetDiscoveredDevices();
  }
  return std::vector<SimpleBLE::Peripheral>();
}

bool BLEDeviceEnumerator::StartScan(int timeoutMs) {
  return pImpl->StartScan(timeoutMs);
}

void BLEDeviceEnumerator::StopScan() { pImpl->StopScan(); }

bool BLEDeviceEnumerator::IsScanning() const { return pImpl->IsScanning(); }

std::vector<SimpleBLE::Peripheral> BLEDeviceEnumerator::GetDiscoveredDevices() {
  return pImpl->GetDiscoveredDevices();
}

void BLEDeviceEnumerator::ClearDiscoveredDevices() {
  pImpl->ClearDiscoveredDevices();
}

std::vector<SimpleBLE::Peripheral>
BLEDeviceEnumerator::FilterDevicesByService(wxString &serviceUuid) {
  std::vector<SimpleBLE::Peripheral> filtered;

  for (auto &device : GetDiscoveredDevices()) {
    if (!device.is_connected())
      device.connect();
    for (auto &service : device.services()) {
      if (service.uuid() == serviceUuid.Upper()) {
        filtered.push_back(device);
        break;
      }
    }
  }

  return filtered;
}

bool BLEDeviceEnumerator::IsBluetoothAvailable() {
  BLEDeviceEnumerator enumerator;
  return enumerator.pImpl->IsBluetoothAvailable();
}
