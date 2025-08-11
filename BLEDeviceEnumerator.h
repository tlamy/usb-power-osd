#ifndef BLEDEVICEENUMERATOR_H
#define BLEDEVICEENUMERATOR_H

#include <wx/wx.h>
#include <vector>
#include <memory>

struct BLEDeviceInfo {
    wxString name; // Device name (if available)
    wxString address; // MAC address
    int rssi; // Signal strength
    bool connectable; // Whether device is connectable
    std::vector<wxString> serviceUuids; // Advertised service UUIDs

    BLEDeviceInfo(const wxString &deviceName = wxEmptyString,
                  const wxString &deviceAddress = wxEmptyString,
                  int signalStrength = -100,
                  bool isConnectable = false)
        : name(deviceName), address(deviceAddress), rssi(signalStrength), connectable(isConnectable) {
    }
};

class BLEDeviceEnumerator {
public:
    BLEDeviceEnumerator();

    ~BLEDeviceEnumerator();

    // Static method to get available BLE devices (similar to SerialPortEnumerator::GetPortNames)
    static std::vector<BLEDeviceInfo> GetBLEDevices(int scanTimeoutMs = 5000);

    // Instance methods for more control
    bool StartScan(int timeoutMs = 5000);

    void StopScan();

    bool IsScanning() const;

    std::vector<BLEDeviceInfo> GetDiscoveredDevices() const;

    void ClearDiscoveredDevices();

    // Filter devices by name pattern or service UUID
    std::vector<BLEDeviceInfo> FilterDevicesByName(const wxString &namePattern) const;

    std::vector<BLEDeviceInfo> FilterDevicesByService(const wxString &serviceUuid) const;

    // Check if BLE adapter is available
    static bool IsBluetoothAvailable();

private:
    class Impl; // PIMPL idiom to hide platform-specific implementation
    std::unique_ptr<Impl> pImpl;
};

#endif // BLEDEVICEENUMERATOR_H
