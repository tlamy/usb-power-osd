// BLEDriver.h
#ifndef BLE_DRIVER_H
#define BLE_DRIVER_H

#define SERVICE_UUID "01bc9d6f-5b93-41bc-b63f-da5011e34f68"
#define CHARACTERISTIC_UUID "307fc9ab-5438-4e03-83fa-b9fc3d6afde2"

#include <functional>
#include <simpleble/SimpleBLE.h>
#include <string>
#include <vector>

class BLEDriver {
public:
  BLEDriver();

  ~BLEDriver();

  // Device discovery
  std::vector<SimpleBLE::Peripheral> scanDevices(int timeout_ms = 5000);

  // Connection management
  bool connect(const std::string &device_address);

  void disconnect();

  bool isConnected() const;

  // Data communication
  bool writeCharacteristic(const std::string &service_uuid,
                           const std::string &characteristic_uuid,
                           const std::vector<uint8_t> &data);

  std::vector<uint8_t>
  readCharacteristic(const std::string &service_uuid,
                     const std::string &characteristic_uuid);

  // Notifications
  bool enableNotifications(
      const std::string &service_uuid, const std::string &characteristic_uuid,
      const std::function<void(const std::vector<uint8_t> &)> &callback);

private:
  SimpleBLE::Adapter adapter;
  SimpleBLE::Peripheral device;
  bool connected = false;
};
inline bool BLEDriver::enableNotifications(
    const std::string &serviceUuid, const std::string &characteristicUuid,
    const std::function<void(const std::vector<uint8_t> &)> &callback) {
  try {
    if (!device.is_connected()) {
      std::cerr << "Device not connected" << std::endl;
      return false;
    }

    // Set up notification callback
    device.notify(serviceUuid, characteristicUuid,
                  [callback](SimpleBLE::ByteArray data) {
                    std::vector<uint8_t> dataVec(data.begin(), data.end());
                    callback(dataVec);
                  });

    std::cout << "Subscribed to notifications" << std::endl;
    return true;

  } catch (const std::exception &e) {
    std::cerr << "Notification subscription error: " << e.what() << std::endl;
    return false;
  }
}

#endif // BLE_DRIVER_H
