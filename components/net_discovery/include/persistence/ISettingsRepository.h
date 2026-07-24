/**
 * @file ISettingsRepository.h
 * @brief Key-Value Settings Repository Interface for NVS/Flash configurations.
 */

#pragma once

#include <string>
#include <optional>
#include <cstdint>

namespace NetDiscovery {
namespace Persistence {

class ISettingsRepository {
public:
    virtual ~ISettingsRepository() = default;

    virtual std::optional<std::string> GetString(const std::string& key) = 0;
    virtual bool SetString(const std::string& key, const std::string& value) = 0;

    virtual std::optional<int32_t> GetInt(const std::string& key) = 0;
    virtual bool SetInt(const std::string& key, int32_t value) = 0;

    virtual std::optional<bool> GetBool(const std::string& key) = 0;
    virtual bool SetBool(const std::string& key, bool value) = 0;

    virtual bool RemoveKey(const std::string& key) = 0;
    virtual bool HasKey(const std::string& key) = 0;
};

} // namespace Persistence
} // namespace NetDiscovery
