//
// Created by Thomas Lamy on 21.08.25.
//

#ifndef USB_METER_OSD_SELECTEDDEVICE_H
#define USB_METER_OSD_SELECTEDDEVICE_H
#include <simpleble/SimpleBLE.h>
#include <wx/wx.h>
enum class DeviceType { Serial, BLE };

struct SelectedDevice {
  DeviceType type;
  wxString deviceInfo;  // Port name for serial, address for BLE
  wxString displayName; // Friendly name for display

  // Add BLE peripheral for connected devices
  SimpleBLE::Peripheral
      blePeripheral; // Only valid when type == DeviceType::BLE
};

#endif // USB_METER_OSD_SELECTEDDEVICE_H