#include <systemd/sd-bus.h>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <map>
#include <memory>

class BluetoothScanner {
private:
    sd_bus* bus = nullptr;
    std::vector<std::string> discovered_devices;

public:
    BluetoothScanner() {
        int r = sd_bus_open_system(&bus);
        if (r < 0) {
            throw std::runtime_error("Failed to connect to system bus: " + std::string(strerror(-r)));
        }
    }

    ~BluetoothScanner() {
        if (bus) {
            sd_bus_unref(bus);
        }
    }

    // Start discovery on the default adapter
    bool startDiscovery() {
        sd_bus_error error = SD_BUS_ERROR_NULL;
        sd_bus_message* reply = nullptr;

        int r = sd_bus_call_method(
                bus,
                "org.bluez",                    // service
                "/org/bluez/hci0",              // object path (default adapter)
                "org.bluez.Adapter1",           // interface
                "StartDiscovery",               // method
                &error,
                &reply,
                ""                              // no parameters
        );

        if (r < 0) {
            std::cerr << "Failed to start discovery: " << error.message << std::endl;
            sd_bus_error_free(&error);
            return false;
        }

        sd_bus_message_unref(reply);
        sd_bus_error_free(&error);
        std::cout << "Discovery started..." << std::endl;
        return true;
    }

    // Stop discovery
    bool stopDiscovery() {
        sd_bus_error error = SD_BUS_ERROR_NULL;
        sd_bus_message* reply = nullptr;

        int r = sd_bus_call_method(
                bus,
                "org.bluez",
                "/org/bluez/hci0",
                "org.bluez.Adapter1",
                "StopDiscovery",
                &error,
                &reply,
                ""
        );

        if (r < 0) {
            std::cerr << "Failed to stop discovery: " << error.message << std::endl;
            sd_bus_error_free(&error);
            return false;
        }

        sd_bus_message_unref(reply);
        sd_bus_error_free(&error);
        std::cout << "Discovery stopped." << std::endl;
        return true;
    }

    // Get device name from device path
    std::string getDeviceName(const std::string& device_path) {
        sd_bus_error error = SD_BUS_ERROR_NULL;
        sd_bus_message* reply = nullptr;

        int r = sd_bus_call_method(
                bus,
                "org.bluez",
                device_path.c_str(),
                "org.freedesktop.DBus.Properties",
                "Get",
                &error,
                &reply,
                "ss",
                "org.bluez.Device1",
                "Name"
        );

        if (r < 0) {
            sd_bus_error_free(&error);
            return "Unknown Device";
        }

        const char* name = nullptr;
        r = sd_bus_message_read(reply, "v", "s", &name);

        std::string device_name = (name && r >= 0) ? name : "Unknown Device";

        sd_bus_message_unref(reply);
        sd_bus_error_free(&error);
        return device_name;
    }

    // Get device address from device path
    std::string getDeviceAddress(const std::string& device_path) {
        sd_bus_error error = SD_BUS_ERROR_NULL;
        sd_bus_message* reply = nullptr;

        int r = sd_bus_call_method(
                bus,
                "org.bluez",
                device_path.c_str(),
                "org.freedesktop.DBus.Properties",
                "Get",
                &error,
                &reply,
                "ss",
                "org.bluez.Device1",
                "Address"
        );

        if (r < 0) {
            sd_bus_error_free(&error);
            return "Unknown Address";
        }

        const char* address = nullptr;
        r = sd_bus_message_read(reply, "v", "s", &address);

        std::string device_address = (address && r >= 0) ? address : "Unknown Address";

        sd_bus_message_unref(reply);
        sd_bus_error_free(&error);
        return device_address;
    }

    // Get all managed objects (devices)
    std::vector<std::string> getDiscoveredDevices() {
        sd_bus_error error = SD_BUS_ERROR_NULL;
        sd_bus_message* reply = nullptr;
        std::vector<std::string> devices;

        int r = sd_bus_call_method(
                bus,
                "org.bluez",
                "/",
                "org.freedesktop.DBus.ObjectManager",
                "GetManagedObjects",
                &error,
                &reply,
                ""
        );

        if (r < 0) {
            std::cerr << "Failed to get managed objects: " << error.message << std::endl;
            sd_bus_error_free(&error);
            return devices;
        }

        // Parse the response to find device objects
        r = sd_bus_message_enter_container(reply, 'a', "{oa{sa{sv}}}");
        if (r < 0) {
            sd_bus_message_unref(reply);
            sd_bus_error_free(&error);
            return devices;
        }

        const char* object_path = nullptr;
        while (sd_bus_message_enter_container(reply, 'e', "oa{sa{sv}}") > 0) {
            r = sd_bus_message_read(reply, "o", &object_path);
            if (r >= 0 && object_path) {
                std::string path(object_path);
                // Check if this is a device object (contains /dev_)
                if (path.find("/org/bluez/hci0/dev_") != std::string::npos) {
                    devices.push_back(path);
                }
            }
            sd_bus_message_skip(reply, "a{sa{sv}}");
            sd_bus_message_exit_container(reply);
        }

        sd_bus_message_exit_container(reply);
        sd_bus_message_unref(reply);
        sd_bus_error_free(&error);

        return devices;
    }

    // Main scanning function
    void scanForDevices(int duration_seconds = 30) {
        std::cout << "Starting Bluetooth device scan for " << duration_seconds << " seconds..." << std::endl;

        if (!startDiscovery()) {
            return;
        }

        // Wait for the specified duration
        std::this_thread::sleep_for(std::chrono::seconds(duration_seconds));

        stopDiscovery();

        // Get discovered devices
        auto devices = getDiscoveredDevices();

        std::cout << "\n=== Discovered Bluetooth Devices ===" << std::endl;
        std::cout << "Found " << devices.size() << " device(s):\n" << std::endl;

        if (devices.empty()) {
            std::cout << "No devices found." << std::endl;
        } else {
            int count = 1;
            for (const auto& device_path : devices) {
                std::string name = getDeviceName(device_path);
                std::string address = getDeviceAddress(device_path);

                std::cout << count++ << ". " << name << std::endl;
                std::cout << "   Address: " << address << std::endl;
                std::cout << "   Path: " << device_path << std::endl;
                std::cout << std::endl;
            }
        }
    }
};

int main() {
    try {
        BluetoothScanner scanner;
        scanner.scanForDevices(30);  // Scan for 30 seconds
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}