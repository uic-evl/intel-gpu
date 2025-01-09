// utils/device_enum.cpp 
#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <thread>
#include <level_zero/ze_api.h>
#include <level_zero/zes_api.h>

struct PowerDomainData {
    zes_pwr_handle_t handle;
    bool isCardLevel;
    int subdeviceId;
    zes_power_energy_counter_t lastCounter;
};

struct DeviceData {
    zes_device_handle_t device;
    std::string name;
    std::string uuid;
    std::vector<PowerDomainData> powerDomains;
};

void printError(const char* funcName, ze_result_t result) {
    std::cerr << "Error in " << funcName << ": ";
    switch (result) {
        case ZE_RESULT_ERROR_UNINITIALIZED:
            std::cerr << "UNINITIALIZED";
            break;
        case ZE_RESULT_ERROR_DEVICE_LOST:
            std::cerr << "DEVICE_LOST";
            break;
        case ZE_RESULT_ERROR_INVALID_NULL_HANDLE:
            std::cerr << "INVALID_NULL_HANDLE";
            break;
        case ZE_RESULT_ERROR_INVALID_NULL_POINTER:
            std::cerr << "INVALID_NULL_POINTER";
            break;
        default:
            std::cerr << "UNKNOWN (" << result << ")";
    }
    std::cerr << std::endl;
}

std::vector<DeviceData> initializeDevices() {
    std::vector<DeviceData> devices;
    ze_result_t result;

    // Initialize SYSMAN
    result = zesInit(0);
    if (result != ZE_RESULT_SUCCESS) {
        printError("zesInit", result);
        return devices;
    }

    // Get driver
    uint32_t driverCount = 0;
    result = zesDriverGet(&driverCount, nullptr);
    if (result != ZE_RESULT_SUCCESS || driverCount == 0) {
        printError("zesDriverGet", result);
        return devices;
    }

    std::vector<zes_driver_handle_t> drivers(driverCount);
    result = zesDriverGet(&driverCount, drivers.data());
    if (result != ZE_RESULT_SUCCESS) {
        printError("zesDriverGet", result);
        return devices;
    }

    // For each driver, get devices
    for (const auto& driver : drivers) {
        uint32_t deviceCount = 0;
        result = zesDeviceGet(driver, &deviceCount, nullptr);
        if (result != ZE_RESULT_SUCCESS || deviceCount == 0) continue;

        std::vector<zes_device_handle_t> deviceHandles(deviceCount);
        result = zesDeviceGet(driver, &deviceCount, deviceHandles.data());
        if (result != ZE_RESULT_SUCCESS) continue;

        for (const auto& deviceHandle : deviceHandles) {
            DeviceData device;
            device.device = deviceHandle;

            // Get device properties
            zes_device_properties_t props = {};
            props.stype = ZES_STRUCTURE_TYPE_DEVICE_PROPERTIES;
            result = zesDeviceGetProperties(deviceHandle, &props);
            if (result != ZE_RESULT_SUCCESS) continue;

            device.name = std::string(props.modelName);
            // Format UUID
            for (int i = 0; i < ZE_MAX_UUID_SIZE; i++) {
                char hex[3];
                snprintf(hex, sizeof(hex), "%02x", props.core.uuid.id[i]);
                device.uuid += hex;
                if (i < ZE_MAX_UUID_SIZE - 1) device.uuid += ":";
            }

            // Get power domains
            uint32_t numPowerDomains = 0;
            result = zesDeviceEnumPowerDomains(deviceHandle, &numPowerDomains, nullptr);
            if (result != ZE_RESULT_SUCCESS || numPowerDomains == 0) continue;

            std::vector<zes_pwr_handle_t> powerHandles(numPowerDomains);
            result = zesDeviceEnumPowerDomains(deviceHandle, &numPowerDomains, powerHandles.data());
            if (result != ZE_RESULT_SUCCESS) continue;

            // Initialize power domains
            for (const auto& powerHandle : powerHandles) {
                zes_power_properties_t powerProps = {};
                powerProps.stype = ZES_STRUCTURE_TYPE_POWER_PROPERTIES;
                result = zesPowerGetProperties(powerHandle, &powerProps);
                if (result != ZE_RESULT_SUCCESS) continue;

                PowerDomainData domain;
                domain.handle = powerHandle;
                domain.isCardLevel = !powerProps.onSubdevice;
                domain.subdeviceId = powerProps.subdeviceId;

                // Get initial energy counter
                result = zesPowerGetEnergyCounter(powerHandle, &domain.lastCounter);
                if (result != ZE_RESULT_SUCCESS) continue;

                device.powerDomains.push_back(domain);
            }

            devices.push_back(device);
        }
    }

    return devices;
}

void monitorPower(std::vector<DeviceData>& devices, int intervalMs = 200) {
    // Track running averages
    while (true) {
        std::cout << "\033[2J\033[1;1H";  // Clear screen
        std::cout << "=== GPU Power Monitoring ===\n";
        std::cout << "Sampling interval: " << intervalMs << "ms\n\n";

        for (auto& device : devices) {
            std::cout << "Device: " << device.name << "\n";
            std::cout << "UUID: " << device.uuid << "\n";

            for (auto& domain : device.powerDomains) {
                zes_power_energy_counter_t currentCounter;
                ze_result_t result = zesPowerGetEnergyCounter(domain.handle, &currentCounter);
                if (result == ZE_RESULT_SUCCESS) {
                    // Handle wraparound
                    if (currentCounter.timestamp < domain.lastCounter.timestamp) {
                        currentCounter.timestamp += UINT64_MAX;
                    }
                    if (currentCounter.energy < domain.lastCounter.energy) {
                        currentCounter.energy += UINT64_MAX;
                    }

                    uint64_t deltaTime = currentCounter.timestamp - domain.lastCounter.timestamp;
                    uint64_t deltaEnergy = currentCounter.energy - domain.lastCounter.energy;

                    if (deltaTime > 0) {
                        // Convert from uJ/us to Watts
                        double power = static_cast<double>(deltaEnergy) / deltaTime;

                        // Format output
                        std::string domainType = domain.isCardLevel ? "Card Total" : 
                                                   "Tile " + std::to_string(domain.subdeviceId);

                        std::cout << std::setw(12) << std::left << domainType 
                                  << " Power: " << std::fixed << std::setprecision(2) 
                                  << std::setw(8) << power << " W"
                                  << "  (Energy: " << std::setprecision(2) 
                                  << (currentCounter.energy / 1e6) << " J)\n";
                    }

                    domain.lastCounter = currentCounter;
                }
            }
            std::cout << "\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
    }
}

int main() {
    auto devices = initializeDevices();
    if (devices.empty()) {
        std::cerr << "No devices found or initialization failed\n";
        return 1;
    }

    monitorPower(devices);
    return 0;
}
