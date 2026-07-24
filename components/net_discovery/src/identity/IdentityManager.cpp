/**
 * @file IdentityManager.cpp
 * @brief Implementation of IdentityManager (v5.0.0 Architecture Phase 18).
 */

#include "identity/IdentityManager.h"

namespace NetDiscovery {
namespace Identity {

std::optional<DeviceIdentityDescriptor> IdentityManager::CreateIdentity(
        const std::string& displayName,
        const std::string& category,
        const std::string& manufacturer,
        const std::string& model) {

    std::lock_guard<std::mutex> lock(m_mutex);
    IdentityId id = "id.device." + std::to_string(m_nextId++);

    DeviceIdentity identity(id, category, manufacturer, model);
    DeviceIdentityContext context(displayName);

    IdentityRecord rec{identity, context};
    m_identities.emplace(id, rec);

    return DeviceIdentityDescriptor(identity, context);
}

std::optional<DeviceIdentityDescriptor> IdentityManager::ResolveIdentity(const std::string& discoveryId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto mapIt = m_discoveryToIdentityMap.find(discoveryId);
    if (mapIt == m_discoveryToIdentityMap.end()) return std::nullopt;

    auto idIt = m_identities.find(mapIt->second);
    if (idIt == m_identities.end()) return std::nullopt;

    return DeviceIdentityDescriptor(idIt->second.identity, idIt->second.context);
}

std::optional<DeviceIdentityDescriptor> IdentityManager::FindIdentity(const IdentityId& identityId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_identities.find(identityId);
    if (it == m_identities.end()) return std::nullopt;
    return DeviceIdentityDescriptor(it->second.identity, it->second.context);
}

bool IdentityManager::LinkDiscoveredDevice(const IdentityId& identityId, const std::string& discoveryId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_identities.find(identityId);
    if (it == m_identities.end()) return false;

    if (!it->second.context.IsLinkedTo(discoveryId)) {
        it->second.context.linkedDiscoveryIds.push_back(discoveryId);
    }
    m_discoveryToIdentityMap.insert_or_assign(discoveryId, identityId);
    return true;
}

bool IdentityManager::UpdateDiscoveryBinding(const IdentityId& identityId,
                                              const std::string& oldDiscoveryId,
                                              const std::string& newDiscoveryId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_identities.find(identityId);
    if (it == m_identities.end()) return false;

    m_discoveryToIdentityMap.erase(oldDiscoveryId);
    m_discoveryToIdentityMap.insert_or_assign(newDiscoveryId, identityId);

    auto& links = it->second.context.linkedDiscoveryIds;
    for (auto& link : links) {
        if (link == oldDiscoveryId) {
            link = newDiscoveryId;
            return true;
        }
    }
    links.push_back(newDiscoveryId);
    return true;
}

bool IdentityManager::RenameIdentity(const IdentityId& identityId, const std::string& newDisplayName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_identities.find(identityId);
    if (it == m_identities.end()) return false;
    it->second.context.displayName = newDisplayName;
    return true;
}

bool IdentityManager::AddAlias(const IdentityId& identityId, const std::string& alias) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_identities.find(identityId);
    if (it == m_identities.end()) return false;
    if (!it->second.context.HasAlias(alias)) {
        it->second.context.aliases.push_back(alias);
    }
    return true;
}

bool IdentityManager::RemoveAlias(const IdentityId& identityId, const std::string& alias) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_identities.find(identityId);
    if (it == m_identities.end()) return false;

    auto& aliases = it->second.context.aliases;
    for (auto aIt = aliases.begin(); aIt != aliases.end(); ++aIt) {
        if (*aIt == alias) {
            aliases.erase(aIt);
            return true;
        }
    }
    return false;
}

bool IdentityManager::AssignRoom(const IdentityId& identityId, const std::string& room) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_identities.find(identityId);
    if (it == m_identities.end()) return false;
    it->second.context.assignedRoom = room;
    return true;
}

bool IdentityManager::RemoveRoom(const IdentityId& identityId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_identities.find(identityId);
    if (it == m_identities.end()) return false;
    it->second.context.assignedRoom.clear();
    return true;
}

bool IdentityManager::Enable(const IdentityId& identityId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_identities.find(identityId);
    if (it == m_identities.end()) return false;
    it->second.context.enabled = true;
    return true;
}

bool IdentityManager::Disable(const IdentityId& identityId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_identities.find(identityId);
    if (it == m_identities.end()) return false;
    it->second.context.enabled = false;
    return true;
}

std::vector<DeviceIdentityDescriptor> IdentityManager::GetAllIdentities() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<DeviceIdentityDescriptor> list;
    list.reserve(m_identities.size());
    for (const auto& [id, rec] : m_identities) {
        list.emplace_back(rec.identity, rec.context);
    }
    return list;
}

} // namespace Identity
} // namespace NetDiscovery
