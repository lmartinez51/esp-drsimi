/**
 * @file ExecutionReporter.h
 * @brief Human-readable execution tracer and report generator (v5.0.0 Architecture Phase 16).
 */

#pragma once

#include "runtime/ExecutionStepResult.h"

#include <string>
#include <vector>

namespace NetDiscovery {
namespace Testing {

struct ExecutionTraceEvent {
    std::string timestamp;
    std::string component;
    std::string eventName;
    std::string details;
};

/**
 * @brief Human-readable execution tracer and report generator.
 */
class ExecutionReporter {
public:
    ExecutionReporter() = default;

    void RecordEvent(std::string component, std::string eventName, std::string details);
    void RecordStepOutcome(const Runtime::ExecutionStepResult& result);

    std::string GenerateReport() const;
    void Clear();

private:
    std::vector<ExecutionTraceEvent> m_events;
};

} // namespace Testing
} // namespace NetDiscovery
