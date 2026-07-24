/**
 * @file MDNSDiscoveryProvider.h
 * @brief mDNS / DNS-SD discovery provider stub (v5.0.0 Architecture Phase 17).
 */

#pragma once

#include "discovery/IDiscoveryProvider.h"
#include <mutex>

namespace NetDiscovery {
namespace Discovery {

class MDNSDiscoveryProvider : public IDiscoveryProvider {
public:
    MDNSDiscoveryProvider() = default;

    std::string GetProviderId() const override { return "provider.mdns"; }
    std::string GetProtocolName() const override { return "mDNS/DNS-SD"; }

    bool StartDiscovery() override { m_isDiscovering = true; return true; }
    void StopDiscovery() override { m_isDiscovering = false; }
    void Refresh() override {}

    bool IsDiscovering() const override { return m_isDiscovering; }
    std::vector<DiscoveredDeviceDescriptor> GetDiscoveredDevices() const override {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_devices;
    }

private:
    bool m_isDiscovering{false};
    mutable std::mutex m_mutex;
    std::vector<DiscoveredDeviceDescriptor> m_devices;
};

} // namespace Discovery
} // namespace NetDiscovery
