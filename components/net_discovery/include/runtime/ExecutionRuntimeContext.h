/**
 * @file ExecutionRuntimeContext.h
 * @brief Mutable runtime execution variable state owned exclusively by ExecutionSession (v5.0.0 Architecture Phase 9.1).
 */

#pragma once

#include "execution/ExecutionPlannerTypes.h"

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <utility>

namespace NetDiscovery {
namespace Runtime {

/**
 * @brief Container holding mutable runtime variables, step outputs, retry counters, and transient data during execution.
 */
class ExecutionRuntimeContext {
public:
    ExecutionRuntimeContext() = default;
    ~ExecutionRuntimeContext() = default;

    ExecutionRuntimeContext(ExecutionRuntimeContext&&) noexcept = default;
    ExecutionRuntimeContext& operator=(ExecutionRuntimeContext&&) noexcept = default;
    ExecutionRuntimeContext(const ExecutionRuntimeContext&) = default;
    ExecutionRuntimeContext& operator=(const ExecutionRuntimeContext&) = default;

    // Retry Counter Operations
    uint32_t GetRetryCount(const Execution::StepId& stepId) const {
        auto it = m_retryCounters.find(stepId);
        return it != m_retryCounters.end() ? it->second : 0;
    }

    void IncrementRetryCount(const Execution::StepId& stepId) {
        m_retryCounters[stepId]++;
    }

    void ResetRetryCount(const Execution::StepId& stepId) {
        m_retryCounters.erase(stepId);
    }

    // Key-Value Runtime Variable Storage
    void SetVariable(const std::string& key, const std::string& value) {
        m_executionVariables[key] = value;
    }

    std::string GetVariable(const std::string& key, const std::string& defaultValue = "") const {
        auto it = m_executionVariables.find(key);
        return it != m_executionVariables.end() ? it->second : defaultValue;
    }

    bool HasVariable(const std::string& key) const {
        return m_executionVariables.find(key) != m_executionVariables.end();
    }

    // Adapter-Local Variables
    void SetAdapterVariable(const std::string& adapterId, const std::string& key, const std::string& value) {
        m_adapterVariables[adapterId + "." + key] = value;
    }

    std::string GetAdapterVariable(const std::string& adapterId, const std::string& key, const std::string& defaultValue = "") const {
        auto it = m_adapterVariables.find(adapterId + "." + key);
        return it != m_adapterVariables.end() ? it->second : defaultValue;
    }

    // Step Output Storage & Retrieval
    void SetStepOutput(const Execution::StepId& stepId, const std::unordered_map<std::string, std::string>& outputs) {
        m_stepOutputs[stepId] = outputs;
    }

    const std::unordered_map<std::string, std::string>* GetStepOutput(const Execution::StepId& stepId) const {
        auto it = m_stepOutputs.find(stepId);
        return it != m_stepOutputs.end() ? &(it->second) : nullptr;
    }

    // Transaction & Control Flags
    const std::string& GetTransactionId() const { return m_transactionId; }
    void SetTransactionId(std::string txId) { m_transactionId = std::move(txId); }

    bool IsCancellationRequested() const { return m_cancellationFlag; }
    void RequestCancellation() { m_cancellationFlag = true; }

    // Timeout Tracking
    void SetStepStartTimestamp(const Execution::StepId& stepId, uint64_t timestampMs) {
        m_stepStartTimestampsMs[stepId] = timestampMs;
    }

    uint64_t GetStepStartTimestamp(const Execution::StepId& stepId) const {
        auto it = m_stepStartTimestampsMs.find(stepId);
        return it != m_stepStartTimestampsMs.end() ? it->second : 0;
    }

    // Accessors
    const std::unordered_map<Execution::StepId, uint32_t>& GetRetryCounters() const { return m_retryCounters; }
    const std::unordered_map<std::string, std::string>& GetExecutionVariables() const { return m_executionVariables; }
    const std::unordered_map<Execution::StepId, std::unordered_map<std::string, std::string>>& GetStepOutputs() const { return m_stepOutputs; }
    const std::unordered_map<std::string, std::string>& GetMetadata() const { return m_metadata; }
    void SetMetadata(const std::string& key, const std::string& value) { m_metadata[key] = value; }

private:
    std::unordered_map<Execution::StepId, uint32_t> m_retryCounters;
    std::unordered_map<std::string, std::string> m_executionVariables;
    std::unordered_map<std::string, std::string> m_adapterVariables;
    std::unordered_map<Execution::StepId, std::unordered_map<std::string, std::string>> m_stepOutputs;
    std::unordered_map<Execution::StepId, uint64_t> m_stepStartTimestampsMs;
    std::unordered_map<std::string, std::string> m_metadata;
    std::string m_transactionId;
    bool m_cancellationFlag{false};
};

} // namespace Runtime
} // namespace NetDiscovery
