/**
 * @file InvocationRequest.h
 * @brief Immutable semantic invocation request model (v5.0.0 Architecture Phase 8.5).
 * 
 * InvocationRequest represents a declarative request to execute a semantic operation 
 * targeting a KnowledgeEntity. It is completely immutable and contains ZERO protocol details.
 */

#pragma once

#include "execution/ExecutionPlannerTypes.h"

#include <string>
#include <unordered_map>
#include <cstdint>

namespace NetDiscovery {
namespace Execution {

/**
 * @brief Immutable metadata model representing a single semantic invocation request.
 */
class InvocationRequest {
public:
    InvocationRequest() = default;

    InvocationRequest(RequestId requestId,
                      std::string targetEntityId,
                      std::string operationId,
                      std::unordered_map<std::string, std::string> parameters = {},
                      uint64_t requestTimestampMs = 0,
                      std::string callerId = "AI.Orchestrator",
                      int priority = 50,
                      uint32_t timeoutMs = 5000,
                      std::string correlationId = "",
                      std::unordered_map<std::string, std::string> metadata = {})
        : m_requestId(std::move(requestId)),
          m_targetEntityId(std::move(targetEntityId)),
          m_operationId(std::move(operationId)),
          m_parameters(std::move(parameters)),
          m_requestTimestampMs(requestTimestampMs),
          m_callerId(std::move(callerId)),
          m_priority(priority),
          m_timeoutMs(timeoutMs),
          m_correlationId(std::move(correlationId)),
          m_metadata(std::move(metadata)) {}

    // Immutable Accessors
    const RequestId& GetRequestId() const { return m_requestId; }
    const std::string& GetTargetEntityId() const { return m_targetEntityId; }
    const std::string& GetOperationId() const { return m_operationId; }
    const std::unordered_map<std::string, std::string>& GetParameters() const { return m_parameters; }
    uint64_t GetRequestTimestampMs() const { return m_requestTimestampMs; }
    const std::string& GetCallerId() const { return m_callerId; }
    int GetPriority() const { return m_priority; }
    uint32_t GetTimeoutMs() const { return m_timeoutMs; }
    const std::string& GetCorrelationId() const { return m_correlationId; }
    const std::unordered_map<std::string, std::string>& GetMetadata() const { return m_metadata; }

    bool operator==(const InvocationRequest& other) const {
        return m_requestId == other.m_requestId;
    }

private:
    RequestId m_requestId;
    std::string m_targetEntityId;
    std::string m_operationId;
    std::unordered_map<std::string, std::string> m_parameters;
    uint64_t m_requestTimestampMs{0};
    std::string m_callerId;
    int m_priority{50};
    uint32_t m_timeoutMs{5000};
    std::string m_correlationId;
    std::unordered_map<std::string, std::string> m_metadata;
};

} // namespace Execution
} // namespace NetDiscovery
