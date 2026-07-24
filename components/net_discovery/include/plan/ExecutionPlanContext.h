/**
 * @file ExecutionPlanContext.h
 * @brief Strongly typed runtime blackboard for ExecutionPlanInstance (v6.0 Phase C).
 */

#pragma once

#include "plan/ExecutionValue.h"
#include <unordered_map>
#include <string>
#include <optional>
#include <mutex>

namespace NetDiscovery {
namespace Plan {

class ExecutionPlanContext {
public:
    ExecutionPlanContext() = default;

    void SetValue(const std::string& key, ExecutionValue value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_variables[key] = std::move(value);
    }

    template<typename T>
    std::optional<T> GetValue(const std::string& key) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_variables.find(key);
        if (it != m_variables.end()) {
            if (const T* val = std::get_if<T>(&it->second)) {
                return *val;
            }
        }
        return std::nullopt;
    }

    std::optional<ExecutionValue> GetRawValue(const std::string& key) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_variables.find(key);
        if (it != m_variables.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    bool Contains(const std::string& key) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_variables.find(key) != m_variables.end();
    }

    void Remove(const std::string& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_variables.erase(key);
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_variables.clear();
    }

    size_t GetSize() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_variables.size();
    }

private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, ExecutionValue> m_variables;
};

} // namespace Plan
} // namespace NetDiscovery
