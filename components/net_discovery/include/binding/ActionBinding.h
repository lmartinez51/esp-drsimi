/**
 * @file ActionBinding.h
 * @brief Immutable metadata mapping semantic operations to protocol implementations (v5.0.0 Architecture Phase 8.1).
 * 
 * ActionBinding represents a declarative, protocol-independent binding definition.
 * It contains ZERO runtime execution logic, callbacks, function pointers, or lambdas.
 * Once constructed, an ActionBinding is treated as immutable metadata.
 */

#pragma once

#include "binding/ParameterBinding.h"
#include "binding/ProtocolAdapterDescriptor.h"
#include "binding/BindingPriority.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace NetDiscovery {
namespace Binding {

using BindingId = std::string;
using OperationId = std::string;

/**
 * @brief Declarative execution profile metadata for an ActionBinding.
 */
struct ExecutionHints {
    uint32_t estimatedDurationMs{0};   // Expected execution latency
    uint32_t timeoutMs{5000};          // Maximum execution timeout before aborting
    uint8_t retryCount{3};             // Maximum automatic retry attempts
    bool requiresWakeup{false};        // True if network/radio wakeup pulse is required
    bool supportsRollback{false};      // True if state rollback on failure is supported
    bool supportsBatch{false};         // True if multiple operations can be batched
    bool exclusiveExecution{false};    // True if hardware lock must be acquired
    bool safe{true};                   // False for destructive actions (e.g. FactoryReset)
    bool requiresConfirmation{false};  // True if UI user confirmation is required
};

/**
 * @brief Immutable metadata representing a declarative binding between a semantic operation and an adapter.
 */
class ActionBinding {
public:
    ActionBinding() = default;

    /**
     * @brief Immutable Constructor initializing all binding metadata fields.
     */
    ActionBinding(BindingId bindingId,
                  OperationId operationId,
                  AdapterId adapterId,
                  std::string protocol,
                  std::string protocolOperation,
                  int priority = PriorityLevel::Default,
                  bool requiresAuth = false,
                  ExecutionHints hints = {},
                  std::vector<ParameterBinding> paramBindings = {},
                  std::unordered_map<std::string, std::string> metadata = {},
                  uint32_t version = 1)
        : m_bindingId(std::move(bindingId)),
          m_operationId(std::move(operationId)),
          m_adapterId(std::move(adapterId)),
          m_protocol(std::move(protocol)),
          m_protocolOperation(std::move(protocolOperation)),
          m_priority(priority),
          m_requiresAuthentication(requiresAuth),
          m_executionHints(std::move(hints)),
          m_parameterBindings(std::move(paramBindings)),
          m_metadata(std::move(metadata)),
          m_version(version) {}

    // Immutable Accessors
    const BindingId& GetBindingId() const { return m_bindingId; }
    const OperationId& GetOperationId() const { return m_operationId; }
    const AdapterId& GetAdapterId() const { return m_adapterId; }
    const std::string& GetProtocol() const { return m_protocol; }
    const std::string& GetProtocolOperation() const { return m_protocolOperation; }
    int GetPriority() const { return m_priority; }
    bool RequiresAuthentication() const { return m_requiresAuthentication; }
    const ExecutionHints& GetExecutionHints() const { return m_executionHints; }
    const std::vector<ParameterBinding>& GetParameterBindings() const { return m_parameterBindings; }
    const std::unordered_map<std::string, std::string>& GetMetadata() const { return m_metadata; }
    uint32_t GetVersion() const { return m_version; }

    bool operator==(const ActionBinding& other) const {
        return m_bindingId == other.m_bindingId && m_version == other.m_version;
    }

private:
    BindingId m_bindingId;
    OperationId m_operationId;
    AdapterId m_adapterId;
    std::string m_protocol;
    std::string m_protocolOperation;
    int m_priority{PriorityLevel::Default};
    bool m_requiresAuthentication{false};
    ExecutionHints m_executionHints;
    std::vector<ParameterBinding> m_parameterBindings;
    std::unordered_map<std::string, std::string> m_metadata;
    uint32_t m_version{1};
};

} // namespace Binding
} // namespace NetDiscovery
