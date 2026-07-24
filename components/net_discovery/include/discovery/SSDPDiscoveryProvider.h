/**
 * @file SSDPDiscoveryProvider.h
 * @brief SSDP / UPnP discovery provider encapsulating existing net_discovery (v5.0.0 Architecture Phase 17).
 */

#pragma once

#include "discovery/IDiscoveryProvider.h"
#include <mutex>
#include <vector>

namespace NetDiscovery {
namespace Discovery {

/**
 * @brief Discovery provider wrapping SSDP / UPnP discovery without modifying existing net_discovery implementation.
 */
class SSDPDiscoveryProvider : public IDiscoveryProvider {
public:
    SSDPDiscoveryProvider() = default;
    ~SSDPDiscoveryProvider() override = default;

    std::string GetProviderId() const override { return "provider.ssdp"; }
    std::string GetProtocolName() const override { return "UPnP/SSDP"; }

    bool StartDiscovery() override;
    void StopDiscovery() override;
    void Refresh() override;

    bool IsDiscovering() const override { return m_isDiscovering; }
    std::vector<DiscoveredDeviceDescriptor> GetDiscoveredDevices() const override;

    void AddSyntheticDiscoveredDevice(DiscoveredDeviceDescriptor device);

private:
    bool m_isDiscovering{false};
    mutable std::mutex m_mutex;
    std::vector<DiscoveredDeviceDescriptor> m_devices;
};

} // namespace Discovery
} // namespace NetDiscovery
