/**
 * @file ManualDiscoveryProvider.cpp
 * @brief Implementation of ManualDiscoveryProvider (v5.0.0 Architecture Phase 17).
 */

#include "discovery/ManualDiscoveryProvider.h"

namespace NetDiscovery {
namespace Discovery {

std::vector<DiscoveredDeviceDescriptor> ManualDiscoveryProvider::GetDiscoveredDevices() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_devices;
}

void ManualDiscoveryProvider::AddManualDevice(DiscoveredDeviceDescriptor device) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_devices.push_back(std::move(device));
}

bool ManualDiscoveryProvider::RemoveManualDevice(const std::string& deviceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it = m_devices.begin(); it != m_devices.end(); ++it) {
        if (it->deviceId == deviceId) {
            m_devices.erase(it);
            return true;
        }
    }
    return false;
}

} // namespace Discovery
} // namespace NetDiscovery
