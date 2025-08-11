// BLEDriver.h
#ifndef BLE_DRIVER_H
#define BLE_DRIVER_H

#define SERVICE_UUID        "01bc9d6f-5b93-41bc-b63f-da5011e34f68"
#define CHARACTERISTIC_UUID "307fc9ab-5438-4e03-83fa-b9fc3d6afde2"

#include <simpleble/SimpleBLE.h>
#include <string>
#include <vector>
#include <functional>

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

    std::vector<uint8_t> readCharacteristic(const std::string &service_uuid,
                                            const std::string &characteristic_uuid);

    // Notifications
    void enableNotifications(const std::string &service_uuid,
                             const std::string &characteristic_uuid,
                             std::function<void(const std::vector<uint8_t> &)> callback);

private:
    SimpleBLE::Adapter adapter_;
    SimpleBLE::Peripheral connected_device_;
    bool connected_ = false;
};

#endif // BLE_DRIVER_H
