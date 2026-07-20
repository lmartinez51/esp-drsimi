/**
 * @file AuthenticationManager.cpp
 * @brief Implementation of AuthenticationManager.
 */

#include "../../include/core/AuthenticationManager.h"
#include <iostream>

namespace NetDiscovery {

AuthenticationManager::AuthenticationManager(KnowledgeStore* store)
    : m_store(store)
{
}

void AuthenticationManager::InjectCredentials(const std::string& deviceId, ExecutionContext& context) {
    if (!m_store) {
        return;
    }

    // Interrogate the KnowledgeStore for credentials for this deviceId
    auto& entities = m_store->GetLoadedEntities();
    for (const auto& entity : entities) {
        if (entity.persistentId == deviceId || entity.lastObservedIdentity == deviceId) {
            for (const auto& kvp : entity.credentials) {
                context.credentials[kvp.first] = kvp.second;
            }
            break;
        }
    }
}

void AuthenticationManager::SaveCredentials(const std::string& deviceId, const std::string& key, const std::string& value) {
    if (m_store) {
        m_store->UpdateCredentials(deviceId, key, value);
    }
}

} // namespace NetDiscovery
