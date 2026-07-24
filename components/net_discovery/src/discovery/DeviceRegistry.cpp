/**
 * @file DeviceRegistry.cpp
 * @brief Implementation of DeviceRegistry (v5.0.0 Architecture Phase 17).
 */

#include "discovery/DeviceRegistry.h"

namespace NetDiscovery {
namespace Discovery {

void DeviceRegistry::RegisterDevice(DiscoveredDeviceDescriptor device) {
    if (!device.IsValid()) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_devices.insert_or_assign(device.deviceId, std::move(device));
}

bool DeviceRegistry::UpdateDevice(DiscoveredDeviceDescriptor device) {
    if (!device.IsValid()) return false;
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_devices.find(device.deviceId);
    if (it == m_devices.end()) return false;
    it->second = std::move(device);
    return true;
}

bool DeviceRegistry::RemoveDevice(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_devices.erase(deviceId) > 0;
}

void DeviceRegistry::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_devices.clear();
}

std::optional<DiscoveredDeviceDescriptor> DeviceRegistry::GetDevice(const std::string& deviceId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_devices.find(deviceId);
    if (it == m_devices.end()) return std::nullopt;
    return it->second;
}

std::vector<DiscoveredDeviceDescriptor> DeviceRegistry::GetAllDevices() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<DiscoveredDeviceDescriptor> list;
    list.reserve(m_devices.size());
    for (const auto& [id, dev] : m_devices) {
        list.push_back(dev);
    }
    return list;
}

std::vector<DiscoveredDeviceDescriptor> DeviceRegistry::GetDevicesByCapability(const std::string& capability) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<DiscoveredDeviceDescriptor> list;
    for (const auto& [id, dev] : m_devices) {
        if (dev.HasCapability(capability)) {
            list.push_back(dev);
        }
    }
    return list;
}

std::vector<DiscoveredDeviceDescriptor> DeviceRegistry::GetDevicesByProtocol(const std::string& protocol) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<DiscoveredDeviceDescriptor> list;
    for (const auto& [id, dev] : m_devices) {
        if (dev.HasProtocol(protocol)) {
            list.push_back(dev);
        }
    }
    return list;
}

std::vector<DiscoveredDeviceDescriptor> DeviceRegistry::GetDevicesByRoom(const std::string& room) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<DiscoveredDeviceDescriptor> list;
    for (const auto& [id, dev] : m_devices) {
        if (dev.room == room) {
            list.push_back(dev);
        }
    }
    return list;
}

std::vector<DiscoveredDeviceDescriptor> DeviceRegistry::GetDevicesByManufacturer(const std::string& manufacturer) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<DiscoveredDeviceDescriptor> list;
    for (const auto& [id, dev] : m_devices) {
        if (dev.manufacturer == manufacturer) {
            list.push_back(dev);
        }
    }
    return list;
}

std::size_t DeviceRegistry::GetCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_devices.size();
}

} // namespace Discovery
} // namespace NetDiscovery
