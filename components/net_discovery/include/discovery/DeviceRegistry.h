/**
 * @file DeviceRegistry.h
 * @brief Thread-safe O(1) device registry storing unified device descriptors (v5.0.0 Architecture Phase 17).
 */

#pragma once

#include "discovery/DiscoveredDeviceDescriptor.h"
#include <unordered_map>
#include <vector>
#include <optional>
#include <mutex>
#include <string>

namespace NetDiscovery {
namespace Discovery {

/**
 * @brief Thread-safe registry maintaining the current active inventory of discovered devices.
 */
class DeviceRegistry {
public:
    DeviceRegistry() = default;
    ~DeviceRegistry() = default;

    DeviceRegistry(const DeviceRegistry&) = delete;
    DeviceRegistry& operator=(const DeviceRegistry&) = delete;

    void RegisterDevice(DiscoveredDeviceDescriptor device);
    bool UpdateDevice(DiscoveredDeviceDescriptor device);
    bool RemoveDevice(const std::string& deviceId);
    void Clear();

    std::optional<DiscoveredDeviceDescriptor> GetDevice(const std::string& deviceId) const;
    std::vector<DiscoveredDeviceDescriptor> GetAllDevices() const;

    std::vector<DiscoveredDeviceDescriptor> GetDevicesByCapability(const std::string& capability) const;
    std::vector<DiscoveredDeviceDescriptor> GetDevicesByProtocol(const std::string& protocol) const;
    std::vector<DiscoveredDeviceDescriptor> GetDevicesByRoom(const std::string& room) const;
    std::vector<DiscoveredDeviceDescriptor> GetDevicesByManufacturer(const std::string& manufacturer) const;

    std::size_t GetCount() const;

private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, DiscoveredDeviceDescriptor> m_devices;
};

} // namespace Discovery
} // namespace NetDiscovery
