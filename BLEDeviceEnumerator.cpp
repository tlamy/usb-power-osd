#include "BLEDeviceEnumerator.h"

#ifdef USE_SIMPLEBLE
#include <simpleble/SimpleBLE.h>
#endif

#include <thread>
#include <chrono>
#include <algorithm>
#include <iostream>

class BLEDeviceEnumerator::Impl {
public:
    Impl() : scanning(false) {
#ifdef USE_SIMPLEBLE
        try {
            auto adapters = SimpleBLE::Adapter::get_adapters();
            if (!adapters.empty()) {
                adapter = std::make_unique<SimpleBLE::Adapter>(adapters[0]);
                std::cout << "BLE Adapter initialized: " << adapter->identifier() << std::endl;
            } else {
                std::cout << "No BLE adapters found" << std::endl;
            }
        } catch (const std::exception &e) {
            std::cerr << "Error initializing BLE adapter: " << e.what() << std::endl;
        }
#else
        std::cout << "SimpleBLE not available (USE_SIMPLEBLE not defined)" << std::endl;
#endif
    }

    ~Impl() {
        StopScan();
    }

    bool StartScan(int timeoutMs) {
#ifdef USE_SIMPLEBLE
        if (!adapter) {
            std::cerr << "No BLE adapter available" << std::endl;
            return false;
        }

        try {
            discoveredDevices.clear();
            scanning = true;

            std::cout << "Setting up BLE scan callback..." << std::endl;
            adapter->set_callback_on_scan_found([this](SimpleBLE::Peripheral peripheral) {
                std::cout << "Found BLE device: " << peripheral.identifier() << " (" << peripheral.address() << ")" <<
                        std::endl;

                BLEDeviceInfo deviceInfo;
                deviceInfo.address = wxString(peripheral.address());

                auto identifier = peripheral.identifier();
                deviceInfo.name = identifier.empty() ? wxT("Unknown Device") : wxString(identifier);

                deviceInfo.rssi = peripheral.rssi();
                deviceInfo.connectable = peripheral.is_connectable();

                // Get advertised services (note: this might not work during scanning for all devices)
                try {
                    auto services = peripheral.services();
                    for (const auto &service: services) {
                        deviceInfo.serviceUuids.push_back(wxString(service.uuid()));
                    }
                } catch (const std::exception &e) {
                    // Services might not be available during scanning
                    std::cout << "Could not get services for " << peripheral.identifier() << ": " << e.what() <<
                            std::endl;
                }

                // Avoid duplicates
                auto it = std::find_if(discoveredDevices.begin(), discoveredDevices.end(),
                                       [&deviceInfo](const BLEDeviceInfo &existing) {
                                           return existing.address == deviceInfo.address;
                                       });

                if (it == discoveredDevices.end()) {
                    discoveredDevices.push_back(deviceInfo);
                } else {
                    // Update existing device info (e.g., RSSI might have changed)
                    it->rssi = deviceInfo.rssi;
                    it->connectable = deviceInfo.connectable;
                }
            });

            std::cout << "Starting BLE scan for " << timeoutMs << "ms..." << std::endl;

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
#else
        std::cerr << "BLE not available: SimpleBLE not compiled in" << std::endl;
        scanning = false;
        return false;
#endif
    }

    void StopScan() {
#ifdef USE_SIMPLEBLE
        if (adapter && scanning) {
            try {
                adapter->scan_stop();
                std::cout << "BLE scan stopped" << std::endl;
            } catch (const std::exception &e) {
                std::cerr << "Error stopping BLE scan: " << e.what() << std::endl;
            }
        }
#endif
        scanning = false;
    }

    bool IsScanning() const {
        return scanning;
    }

    std::vector<BLEDeviceInfo> GetDiscoveredDevices() const {
        return discoveredDevices;
    }

    void ClearDiscoveredDevices() {
        discoveredDevices.clear();
    }

    bool IsBluetoothAvailable() const {
#ifdef USE_SIMPLEBLE
        return adapter != nullptr;
#else
        return false;
#endif
    }

private:
#ifdef USE_SIMPLEBLE
    std::unique_ptr<SimpleBLE::Adapter> adapter;
#endif
    std::vector<BLEDeviceInfo> discoveredDevices;
    std::atomic<bool> scanning; // Make it atomic for thread safety
};

// BLEDeviceEnumerator implementation
BLEDeviceEnumerator::BLEDeviceEnumerator()
    : pImpl(std::make_unique<Impl>()) {
}

BLEDeviceEnumerator::~BLEDeviceEnumerator() = default;

std::vector<BLEDeviceInfo> BLEDeviceEnumerator::GetBLEDevices(int scanTimeoutMs) {
    BLEDeviceEnumerator enumerator;
    if (enumerator.StartScan(scanTimeoutMs)) {
        // Wait for scan to complete
        std::this_thread::sleep_for(std::chrono::milliseconds(scanTimeoutMs + 100));
        return enumerator.GetDiscoveredDevices();
    }
    return std::vector<BLEDeviceInfo>();
}

bool BLEDeviceEnumerator::StartScan(int timeoutMs) {
    return pImpl->StartScan(timeoutMs);
}

void BLEDeviceEnumerator::StopScan() {
    pImpl->StopScan();
}

bool BLEDeviceEnumerator::IsScanning() const {
    return pImpl->IsScanning();
}

std::vector<BLEDeviceInfo> BLEDeviceEnumerator::GetDiscoveredDevices() const {
    return pImpl->GetDiscoveredDevices();
}

void BLEDeviceEnumerator::ClearDiscoveredDevices() {
    pImpl->ClearDiscoveredDevices();
}

std::vector<BLEDeviceInfo> BLEDeviceEnumerator::FilterDevicesByName(const wxString &namePattern) const {
    auto devices = GetDiscoveredDevices();
    std::vector<BLEDeviceInfo> filtered;

    for (const auto &device: devices) {
        if (device.name.Contains(namePattern)) {
            filtered.push_back(device);
        }
    }

    return filtered;
}

std::vector<BLEDeviceInfo> BLEDeviceEnumerator::FilterDevicesByService(const wxString &serviceUuid) const {
    auto devices = GetDiscoveredDevices();
    std::vector<BLEDeviceInfo> filtered;

    for (const auto &device: devices) {
        for (const auto &uuid: device.serviceUuids) {
            if (uuid.Upper() == serviceUuid.Upper()) {
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
