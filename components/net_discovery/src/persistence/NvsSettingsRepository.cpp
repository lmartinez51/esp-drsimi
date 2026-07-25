/**
 * @file NvsSettingsRepository.cpp
 * @brief NVS key-value implementation of ISettingsRepository.
 */

#include "persistence/NvsSettingsRepository.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

// static const char* TAG = "NvsSettingsRepo";

namespace NetDiscovery
{
    namespace Persistence
    {

        NvsSettingsRepository::NvsSettingsRepository(
            std::shared_ptr<RepositoryContext> context,
            std::string nvsNamespace)
            : m_context(context ? context : std::make_shared<RepositoryContext>()),
              m_namespace(std::move(nvsNamespace)) {}

        std::optional<std::string> NvsSettingsRepository::GetString(const std::string &key)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            nvs_handle_t handle;
            if (nvs_open(m_namespace.c_str(), NVS_READONLY, &handle) != ESP_OK)
                return std::nullopt;

            size_t requiredLen = 0;
            if (nvs_get_str(handle, key.c_str(), nullptr, &requiredLen) != ESP_OK)
            {
                nvs_close(handle);
                return std::nullopt;
            }

            std::string value(requiredLen, '\0');
            esp_err_t err = nvs_get_str(handle, key.c_str(), &value[0], &requiredLen);
            nvs_close(handle);

            if (err == ESP_OK)
            {
                if (!value.empty() && value.back() == '\0')
                    value.pop_back();
                return value;
            }
            return std::nullopt;
        }

        bool NvsSettingsRepository::SetString(const std::string &key, const std::string &value)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            nvs_handle_t handle;
            if (nvs_open(m_namespace.c_str(), NVS_READWRITE, &handle) != ESP_OK)
                return false;

            esp_err_t err = nvs_set_str(handle, key.c_str(), value.c_str());
            if (err == ESP_OK)
                err = nvs_commit(handle);
            nvs_close(handle);

            return (err == ESP_OK);
        }

        std::optional<int32_t> NvsSettingsRepository::GetInt(const std::string &key)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            nvs_handle_t handle;
            if (nvs_open(m_namespace.c_str(), NVS_READONLY, &handle) != ESP_OK)
                return std::nullopt;

            int32_t val = 0;
            esp_err_t err = nvs_get_i32(handle, key.c_str(), &val);
            nvs_close(handle);

            if (err == ESP_OK)
                return val;
            return std::nullopt;
        }

        bool NvsSettingsRepository::SetInt(const std::string &key, int32_t value)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            nvs_handle_t handle;
            if (nvs_open(m_namespace.c_str(), NVS_READWRITE, &handle) != ESP_OK)
                return false;

            esp_err_t err = nvs_set_i32(handle, key.c_str(), value);
            if (err == ESP_OK)
                err = nvs_commit(handle);
            nvs_close(handle);

            return (err == ESP_OK);
        }

        std::optional<bool> NvsSettingsRepository::GetBool(const std::string &key)
        {
            auto res = GetInt(key);
            if (res.has_value())
                return (res.value() != 0);
            return std::nullopt;
        }

        bool NvsSettingsRepository::SetBool(const std::string &key, bool value)
        {
            return SetInt(key, value ? 1 : 0);
        }

        bool NvsSettingsRepository::RemoveKey(const std::string &key)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            nvs_handle_t handle;
            if (nvs_open(m_namespace.c_str(), NVS_READWRITE, &handle) != ESP_OK)
                return false;

            esp_err_t err = nvs_erase_key(handle, key.c_str());
            if (err == ESP_OK)
                err = nvs_commit(handle);
            nvs_close(handle);

            return (err == ESP_OK);
        }

        bool NvsSettingsRepository::HasKey(const std::string &key)
        {
            return GetString(key).has_value() || GetInt(key).has_value();
        }

        bool NvsSettingsRepository::ContainsKey(const std::string &key)
        {
            return HasKey(key);
        }

    } // namespace Persistence
} // namespace NetDiscovery
