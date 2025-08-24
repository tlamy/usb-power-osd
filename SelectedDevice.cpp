#include "SelectedDevice.h"

// Constructor for Serial devices
SelectedDevice::SelectedDevice(DeviceType type, const wxString& deviceInfo, const wxString& displayName)
    : type(type), deviceInfo(deviceInfo), displayName(displayName) {
    // blePeripheral will be default-constructed (empty/invalid)
}

// Constructor for BLE devices with peripheral
SelectedDevice::SelectedDevice(DeviceType type, const wxString& deviceInfo, const wxString& displayName,
                               const SimpleBLE::Peripheral& blePeripheral)
    : type(type), deviceInfo(deviceInfo), displayName(displayName), blePeripheral(blePeripheral) {
}
