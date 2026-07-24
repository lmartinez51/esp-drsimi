/**
 * @file ManualDiscoveryProvider.h
 * @brief Manual/Static configuration device discovery provider (v5.0.0 Architecture Phase 17).
 */

#pragma once

#include "discovery/IDiscoveryProvider.h"
#include <mutex>

namespace NetDiscovery {
namespace Discovery {

/**
 * @brief Discovery provider managing manually/statically configured devices.
 */
class ManualDiscoveryProvider : public IDiscoveryProvider {
public:
    ManualDiscoveryProvider() = default;

    std::string GetProviderId() const override { return "provider.manual"; }
    std::string GetProtocolName() const override { return "Manual"; }

    bool StartDiscovery() override { m_isDiscovering = true; return true; }
    void StopDiscovery() override { m_isDiscovering = false; }
    void Refresh() override {}

    bool IsDiscovering() const override { return m_isDiscovering; }
    std::vector<DiscoveredDeviceDescriptor> GetDiscoveredDevices() const override;

    void AddManualDevice(DiscoveredDeviceDescriptor device);
    bool RemoveManualDevice(const std::string& deviceId);

private:
    bool m_isDiscovering{true};
    mutable std::mutex m_mutex;
    std::vector<DiscoveredDeviceDescriptor> m_devices;
};

} // namespace Discovery
} // namespace NetDiscovery
