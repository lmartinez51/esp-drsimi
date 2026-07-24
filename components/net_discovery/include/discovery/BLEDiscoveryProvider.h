/**
 * @file BLEDiscoveryProvider.h
 * @brief BLE advertisement discovery provider stub (v5.0.0 Architecture Phase 17).
 */

#pragma once

#include "discovery/IDiscoveryProvider.h"
#include <mutex>

namespace NetDiscovery {
namespace Discovery {

class BLEDiscoveryProvider : public IDiscoveryProvider {
public:
    BLEDiscoveryProvider() = default;

    std::string GetProviderId() const override { return "provider.ble"; }
    std::string GetProtocolName() const override { return "BLE"; }

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
