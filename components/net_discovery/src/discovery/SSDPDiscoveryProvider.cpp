/**
 * @file SSDPDiscoveryProvider.cpp
 * @brief Implementation of SSDPDiscoveryProvider (v5.0.0 Architecture Phase 17).
 */

#include "discovery/SSDPDiscoveryProvider.h"

namespace NetDiscovery {
namespace Discovery {

bool SSDPDiscoveryProvider::StartDiscovery() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isDiscovering = true;
    return true;
}

void SSDPDiscoveryProvider::StopDiscovery() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isDiscovering = false;
}

void SSDPDiscoveryProvider::Refresh() {
    // Encapsulated refresh trigger
}

std::vector<DiscoveredDeviceDescriptor> SSDPDiscoveryProvider::GetDiscoveredDevices() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_devices;
}

void SSDPDiscoveryProvider::AddSyntheticDiscoveredDevice(DiscoveredDeviceDescriptor device) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_devices.push_back(std::move(device));
}

} // namespace Discovery
} // namespace NetDiscovery
