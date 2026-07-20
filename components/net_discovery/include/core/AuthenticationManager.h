/**
 * @file AuthenticationManager.h
 * @brief Retrieves authentication credentials for devices from KnowledgeStore.
 */

#pragma once

#include "../services/KnowledgeStore.h"
#include "ExecutionContext.h"
#include <memory>
#include <string>

namespace NetDiscovery {

/**
 * @brief Manages authentication state and token injection.
 */
class AuthenticationManager {
public:
    AuthenticationManager(KnowledgeStore* store);

    /**
     * @brief Interrogates the KnowledgeStore for credentials and injects them into the ExecutionContext.
     */
    void InjectCredentials(const std::string& deviceId, ExecutionContext& context);

    /**
     * @brief Saves a new authentication token back to the KnowledgeStore.
     */
    void SaveCredentials(const std::string& deviceId, const std::string& key, const std::string& value);

private:
    KnowledgeStore* m_store;
};

} // namespace NetDiscovery
