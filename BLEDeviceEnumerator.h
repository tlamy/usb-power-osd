#ifndef BLEDEVICEENUMERATOR_H
#define BLEDEVICEENUMERATOR_H

#include "simpleble/Peripheral.h"

#include <memory>
#include <vector>
#include <wx/wx.h>

class BLEDeviceEnumerator {
public:
  BLEDeviceEnumerator();

  ~BLEDeviceEnumerator();

  // Static method to get available BLE devices (similar to
  // SerialPortEnumerator::GetPortNames)
  static std::vector<SimpleBLE::Peripheral> GetBLEDevices(int scanTimeoutMs = 5000);

  // Instance methods for more control
  bool StartScan(int timeoutMs = 5000);

  void StopScan();

  bool IsScanning() const;

  std::vector<SimpleBLE::Peripheral> GetDiscoveredDevices();

  void ClearDiscoveredDevices();

  std::vector<SimpleBLE::Peripheral>
  FilterDevicesByService(wxString &serviceUuid);

  // Check if BLE adapter is available
  static bool IsBluetoothAvailable();

private:
  class Impl; // PIMPL idiom to hide platform-specific implementation
  std::unique_ptr<Impl> pImpl;
};

#endif // BLEDEVICEENUMERATOR_H
