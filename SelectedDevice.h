//
// Created by Thomas Lamy on 21.08.25.
//

#ifndef USB_METER_OSD_SELECTEDDEVICE_H
#define USB_METER_OSD_SELECTEDDEVICE_H
#include <simpleble/SimpleBLE.h>
#include <wx/wx.h>
enum class DeviceType { Serial, BLE };

class SelectedDevice {
public:
  // Constructor for Serial devices
  SelectedDevice(DeviceType type, const wxString& deviceInfo, const wxString& displayName);
  
  // Constructor for BLE devices with peripheral
  SelectedDevice(DeviceType type, const wxString& deviceInfo, const wxString& displayName, 
                 const SimpleBLE::Peripheral& blePeripheral);

  // Getters
  [[nodiscard]] DeviceType getType() const { return type; }
  [[nodiscard]] const wxString& getDeviceInfo() const { return deviceInfo; }
  [[nodiscard]] const wxString& getDisplayName() const { return displayName; }
  [[nodiscard]] const SimpleBLE::Peripheral& getBlePeripheral() const { return blePeripheral; }
  
  // Setters
  void setType(DeviceType newType) { type = newType; }
  void setDeviceInfo(const wxString& newDeviceInfo) { deviceInfo = newDeviceInfo; }
  void setDisplayName(const wxString& newDisplayName) { displayName = newDisplayName; }
  void setBlePeripheral(const SimpleBLE::Peripheral& newBlePeripheral) { blePeripheral = newBlePeripheral; }

private:
  DeviceType type;
  wxString deviceInfo;  // Port name for serial, address for BLE
  wxString displayName; // Friendly name for display

  // Add BLE peripheral for connected devices
  SimpleBLE::Peripheral
      blePeripheral; // Only valid when type == DeviceType::BLE
};

#endif // USB_METER_OSD_SELECTEDDEVICE_H