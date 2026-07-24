/**
 * @file ExecutionStep.h
 * @brief Immutable representation of a single executable step in an ExecutionPlan (v5.0.0 Architecture Phase 13).
 * 
 * ExecutionStep describes a planned operation step bound to an ActionBinding and adapter.
 * Contains ZERO execution logic or callbacks.
 */

#pragma once

#include "execution/ExecutionPlannerTypes.h"
#include "protocol/capability/ProtocolCapabilityRequirement.h"

#include <string>
#include <unordered_map>
#include <optional>
#include <cstdint>

namespace NetDiscovery {
namespace Execution {

/**
 * @brief Immutable executable step within an ExecutionPlan graph.
 */
class ExecutionStep {
public:
    ExecutionStep() = default;

    ExecutionStep(StepId stepId,
                  std::string bindingId,
                  std::string adapterId,
                  std::string operationId,
                  std::unordered_map<std::string, std::string> parameterValues = {},
                  uint32_t estimatedDurationMs = 100,
                  uint32_t timeoutMs = 5000,
                  std::optional<StepId> rollbackStepId = std::nullopt,
                  bool optional = false,
                  int parallelGroup = 0,
                  std::unordered_map<std::string, std::string> metadata = {},
                  Protocol::ProtocolCapabilityRequirement capabilityRequirement = {})
        : m_stepId(std::move(stepId)),
          m_bindingId(std::move(bindingId)),
          m_adapterId(std::move(adapterId)),
          m_operationId(std::move(operationId)),
          m_parameterValues(std::move(parameterValues)),
          m_estimatedDurationMs(estimatedDurationMs),
          m_timeoutMs(timeoutMs),
          m_rollbackStepId(std::move(rollbackStepId)),
          m_optional(optional),
          m_parallelGroup(parallelGroup),
          m_metadata(std::move(metadata)),
          m_capabilityRequirement(std::move(capabilityRequirement)) {}

    // Immutable Accessors
    const StepId& GetStepId() const { return m_stepId; }
    const std::string& GetBindingId() const { return m_bindingId; }
    const std::string& GetAdapterId() const { return m_adapterId; }
    const std::string& GetOperationId() const { return m_operationId; }
    const std::unordered_map<std::string, std::string>& GetParameterValues() const { return m_parameterValues; }
    uint32_t GetEstimatedDurationMs() const { return m_estimatedDurationMs; }
    uint32_t GetTimeoutMs() const { return m_timeoutMs; }
    const std::optional<StepId>& GetRollbackStepId() const { return m_rollbackStepId; }
    bool IsOptional() const { return m_optional; }
    int GetParallelGroup() const { return m_parallelGroup; }
    const std::unordered_map<std::string, std::string>& GetMetadata() const { return m_metadata; }
    const Protocol::ProtocolCapabilityRequirement& GetCapabilityRequirement() const { return m_capabilityRequirement; }

    bool operator==(const ExecutionStep& other) const {
        return m_stepId == other.m_stepId;
    }

private:
    StepId m_stepId;
    std::string m_bindingId;
    std::string m_adapterId;
    std::string m_operationId;
    std::unordered_map<std::string, std::string> m_parameterValues;
    uint32_t m_estimatedDurationMs{100};
    uint32_t m_timeoutMs{5000};
    std::optional<StepId> m_rollbackStepId;
    bool m_optional{false};
    int m_parallelGroup{0};
    std::unordered_map<std::string, std::string> m_metadata;
    Protocol::ProtocolCapabilityRequirement m_capabilityRequirement;
};

} // namespace Execution
} // namespace NetDiscovery
