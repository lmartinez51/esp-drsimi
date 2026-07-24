/**
 * @file IdentityManager.h
 * @brief Thread-safe implementation of IIdentityManager (v5.0.0 Architecture Phase 18).
 */

#pragma once

#include "identity/IIdentityManager.h"
#include <mutex>
#include <unordered_map>

namespace NetDiscovery {
namespace Identity {

/**
 * @brief Memory-only thread-safe implementation of IIdentityManager.
 *
 * Owns logical device identities and mappings from physical discovery identifiers to logical identities.
 */
class IdentityManager : public IIdentityManager {
public:
    IdentityManager() = default;
    ~IdentityManager() override = default;

    std::optional<DeviceIdentityDescriptor> CreateIdentity(
        const std::string& displayName,
        const std::string& category,
        const std::string& manufacturer = "",
        const std::string& model = "") override;

    std::optional<DeviceIdentityDescriptor> ResolveIdentity(const std::string& discoveryId) const override;
    std::optional<DeviceIdentityDescriptor> FindIdentity(const IdentityId& identityId) const override;

    bool LinkDiscoveredDevice(const IdentityId& identityId, const std::string& discoveryId) override;
    bool UpdateDiscoveryBinding(const IdentityId& identityId, const std::string& oldDiscoveryId, const std::string& newDiscoveryId) override;

    bool RenameIdentity(const IdentityId& identityId, const std::string& newDisplayName) override;
    bool AddAlias(const IdentityId& identityId, const std::string& alias) override;
    bool RemoveAlias(const IdentityId& identityId, const std::string& alias) override;

    bool AssignRoom(const IdentityId& identityId, const std::string& room) override;
    bool RemoveRoom(const IdentityId& identityId) override;

    bool Enable(const IdentityId& identityId) override;
    bool Disable(const IdentityId& identityId) override;

    std::vector<DeviceIdentityDescriptor> GetAllIdentities() const override;

private:
    struct IdentityRecord {
        DeviceIdentity        identity;
        DeviceIdentityContext context;
    };

    mutable std::mutex m_mutex;
    std::unordered_map<IdentityId, IdentityRecord> m_identities;
    std::unordered_map<std::string, IdentityId>    m_discoveryToIdentityMap;
    uint64_t m_nextId{1};
};

} // namespace Identity
} // namespace NetDiscovery
