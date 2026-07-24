/**
 * @file MatterDiscoveryProvider.h
 * @brief Matter operational discovery provider stub (v5.0.0 Architecture Phase 17).
 */

#pragma once

#include "discovery/IDiscoveryProvider.h"
#include <mutex>

namespace NetDiscovery {
namespace Discovery {

class MatterDiscoveryProvider : public IDiscoveryProvider {
public:
    MatterDiscoveryProvider() = default;

    std::string GetProviderId() const override { return "provider.matter"; }
    std::string GetProtocolName() const override { return "Matter"; }

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
