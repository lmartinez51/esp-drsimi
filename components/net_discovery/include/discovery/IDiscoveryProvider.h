/**
 * @file IDiscoveryProvider.h
 * @brief Pure interface for network technology discovery providers (v5.0.0 Architecture Phase 17).
 */

#pragma once

#include "discovery/DiscoveredDeviceDescriptor.h"
#include <vector>
#include <string>

namespace NetDiscovery {
namespace Discovery {

/**
 * @brief Pure abstract interface implemented by concrete discovery providers (SSDP, mDNS, BLE, Matter, Manual).
 */
class IDiscoveryProvider {
public:
    virtual ~IDiscoveryProvider() = default;

    virtual std::string GetProviderId() const = 0;
    virtual std::string GetProtocolName() const = 0;

    virtual bool StartDiscovery() = 0;
    virtual void StopDiscovery() = 0;
    virtual void Refresh() = 0;

    virtual bool IsDiscovering() const = 0;
    virtual std::vector<DiscoveredDeviceDescriptor> GetDiscoveredDevices() const = 0;
};

} // namespace Discovery
} // namespace NetDiscovery
