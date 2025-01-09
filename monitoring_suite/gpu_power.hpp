#pragma once
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include <level_zero/ze_api.h>
#include <level_zero/zes_api.h>

struct GPUPowerData {
    std::string gpu_name;
    std::string uuid;
    double card_power;
    double tile0_power;
    double tile1_power;
};

class GPUPowerMonitor {
private:
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

    std::vector<DeviceData> devices;
    bool initialized;

    void printError(const char* funcName, ze_result_t result) {
        std::cerr << "Error in " << funcName << ": " << result << std::endl;
    }

    bool initializeDevices() {
        ze_result_t result;

        // Initialize SYSMAN
        result = zesInit(0);
        if (result != ZE_RESULT_SUCCESS) {
            printError("zesInit", result);
            return false;
        }

        // Get driver
        uint32_t driverCount = 0;
        result = zesDriverGet(&driverCount, nullptr);
        if (result != ZE_RESULT_SUCCESS || driverCount == 0) {
            printError("zesDriverGet", result);
            return false;
        }

        std::vector<zes_driver_handle_t> drivers(driverCount);
        result = zesDriverGet(&driverCount, drivers.data());
        if (result != ZE_RESULT_SUCCESS) {
            printError("zesDriverGet", result);
            return false;
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

        return !devices.empty();
    }

public:
    GPUPowerMonitor() : initialized(false) {}

    bool initialize() {
        if (!initialized) {
            initialized = initializeDevices();
        }
        return initialized;
    }

    std::vector<GPUPowerData> getPowerReadings() {
        std::vector<GPUPowerData> readings;
        if (!initialized) return readings;

        for (auto& device : devices) {
            GPUPowerData data;
            data.gpu_name = device.name;
            data.uuid = device.uuid;
            data.card_power = -1;
            data.tile0_power = -1;
            data.tile1_power = -1;

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
                        double power = static_cast<double>(deltaEnergy) / deltaTime;
                        
                        if (domain.isCardLevel) {
                            data.card_power = power;
                        } else if (domain.subdeviceId == 0) {
                            data.tile0_power = power;
                        } else if (domain.subdeviceId == 1) {
                            data.tile1_power = power;
                        }
                    }

                    domain.lastCounter = currentCounter;
                }
            }
            readings.push_back(data);
        }

        return readings;
    }
};