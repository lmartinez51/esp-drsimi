/**
 * @file NvsSettingsRepository.h
 * @brief NVS key-value implementation of ISettingsRepository using RepositoryContext.
 */

#pragma once

#include "persistence/ISettingsRepository.h"
#include "persistence/RepositoryContext.h"
#include <memory>
#include <mutex>

namespace NetDiscovery {
namespace Persistence {

class NvsSettingsRepository : public ISettingsRepository {
public:
    explicit NvsSettingsRepository(
        std::shared_ptr<RepositoryContext> context = nullptr,
        std::string nvsNamespace = "settings");
    ~NvsSettingsRepository() override = default;

    std::optional<std::string> GetString(const std::string& key) override;
    bool SetString(const std::string& key, const std::string& value) override;

    std::optional<int32_t> GetInt(const std::string& key) override;
    bool SetInt(const std::string& key, int32_t value) override;

    std::optional<bool> GetBool(const std::string& key) override;
    bool SetBool(const std::string& key, bool value) override;

    bool RemoveKey(const std::string& key) override;
    bool HasKey(const std::string& key) override;
    bool ContainsKey(const std::string& key);

private:
    std::shared_ptr<RepositoryContext> m_context;
    std::string m_namespace;
    mutable std::mutex m_mutex;
};

} // namespace Persistence
} // namespace NetDiscovery
