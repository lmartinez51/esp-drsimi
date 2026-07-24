/**
 * @file RepositoryContext.h
 * @brief Shared Infrastructure Context for ESP-Claw Persistence Repositories.
 */

#pragma once

#include <string>
#include <mutex>
#include <memory>
#include <cstdint>

namespace NetDiscovery {
namespace Persistence {

/**
 * @brief Shared infrastructure context providing base storage paths, synchronization primitives,
 * time services, and logging utilities to persistence repositories.
 */
class RepositoryContext {
public:
    explicit RepositoryContext(std::string basePath = "/littlefs");
    ~RepositoryContext() = default;

    // Path Utilities
    const std::string& GetBasePath() const { return m_basePath; }
    std::string GetKnowledgePath() const { return m_basePath + "/knowledge"; }
    std::string GetResourcesPath() const { return m_basePath + "/resources"; }
    std::string GetSettingsNamespace() const { return "settings"; }

    // Time Service
    int64_t GetCurrentTimestamp() const;

    // Mutex Access for Cross-Repository Operations
    std::mutex& GetGlobalMutex() const { return m_globalMutex; }

private:
    std::string m_basePath;
    mutable std::mutex m_globalMutex;
};

} // namespace Persistence
} // namespace NetDiscovery
