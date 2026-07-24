/**
 * @file IIdentityManager.h
 * @brief Pure interface for device identity management (v5.0.0 Architecture Phase 18).
 */

#pragma once

#include "identity/DeviceIdentity.h"
#include "identity/DeviceIdentityContext.h"
#include "identity/DeviceIdentityDescriptor.h"
#include "identity/IdentityEvent.h"

#include <optional>
#include <vector>
#include <string>

namespace NetDiscovery {
namespace Identity {

/**
 * @brief Pure abstract interface for managing logical device identities.
 */
class IIdentityManager {
public:
    virtual ~IIdentityManager() = default;

    virtual std::optional<DeviceIdentityDescriptor> CreateIdentity(
        const std::string& displayName,
        const std::string& category,
        const std::string& manufacturer = "",
        const std::string& model = "") = 0;

    virtual std::optional<DeviceIdentityDescriptor> ResolveIdentity(const std::string& discoveryId) const = 0;
    virtual std::optional<DeviceIdentityDescriptor> FindIdentity(const IdentityId& identityId) const = 0;

    virtual bool LinkDiscoveredDevice(const IdentityId& identityId, const std::string& discoveryId) = 0;
    virtual bool UpdateDiscoveryBinding(const IdentityId& identityId, const std::string& oldDiscoveryId, const std::string& newDiscoveryId) = 0;

    virtual bool RenameIdentity(const IdentityId& identityId, const std::string& newDisplayName) = 0;
    virtual bool AddAlias(const IdentityId& identityId, const std::string& alias) = 0;
    virtual bool RemoveAlias(const IdentityId& identityId, const std::string& alias) = 0;

    virtual bool AssignRoom(const IdentityId& identityId, const std::string& room) = 0;
    virtual bool RemoveRoom(const IdentityId& identityId) = 0;

    virtual bool Enable(const IdentityId& identityId) = 0;
    virtual bool Disable(const IdentityId& identityId) = 0;

    virtual std::vector<DeviceIdentityDescriptor> GetAllIdentities() const = 0;
};

} // namespace Identity
} // namespace NetDiscovery
