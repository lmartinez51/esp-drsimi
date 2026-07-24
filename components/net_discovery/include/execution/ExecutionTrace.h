/**
 * @file ExecutionTrace.h
 * @brief Diagnostic trace model for execution planning decisions (v5.0.0 Architecture Phase 8.5).
 * 
 * Captures decision rationale, candidate scoring traces, and dependency trees for 
 * AI reasoning and platform diagnostics.
 */

#pragma once

#include "execution/ExecutionPlannerTypes.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace NetDiscovery {
namespace Execution {

/**
 * @brief Diagnostic trace model recording planning rationale.
 */
class ExecutionTrace {
public:
    ExecutionTrace() = default;

    ExecutionTrace(TraceId traceId,
                   RequestId requestId,
                   PlanId planId,
                   uint64_t creationTimestampMs = 0,
                   std::vector<std::string> decisionLog = {},
                   std::unordered_map<std::string, std::string> metadata = {})
        : m_traceId(std::move(traceId)),
          m_requestId(std::move(requestId)),
          m_planId(std::move(planId)),
          m_creationTimestampMs(creationTimestampMs),
          m_decisionLog(std::move(decisionLog)),
          m_metadata(std::move(metadata)) {}

    const TraceId& GetTraceId() const { return m_traceId; }
    const RequestId& GetRequestId() const { return m_requestId; }
    const PlanId& GetPlanId() const { return m_planId; }
    uint64_t GetCreationTimestampMs() const { return m_creationTimestampMs; }
    const std::vector<std::string>& GetDecisionLog() const { return m_decisionLog; }
    const std::unordered_map<std::string, std::string>& GetMetadata() const { return m_metadata; }

    void AddDecision(std::string entry) {
        m_decisionLog.push_back(std::move(entry));
    }

private:
    TraceId m_traceId;
    RequestId m_requestId;
    PlanId m_planId;
    uint64_t m_creationTimestampMs{0};
    std::vector<std::string> m_decisionLog;
    std::unordered_map<std::string, std::string> m_metadata;
};

} // namespace Execution
} // namespace NetDiscovery
