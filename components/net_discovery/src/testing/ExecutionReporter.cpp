/**
 * @file ExecutionReporter.cpp
 * @brief Implementation of ExecutionReporter (v5.0.0 Architecture Phase 16).
 */

#include "testing/ExecutionReporter.h"

namespace NetDiscovery {
namespace Testing {

void ExecutionReporter::RecordEvent(std::string component, std::string eventName, std::string details) {
    m_events.push_back({"0ms", std::move(component), std::move(eventName), std::move(details)});
}

void ExecutionReporter::RecordStepOutcome(const Runtime::ExecutionStepResult& result) {
    std::string details = "Status=" + Execution::ToString(result.status) +
                          ", Adapter=" + result.adapterId +
                          ", Duration=" + std::to_string(result.executionDurationMs) + "ms";
    if (!result.IsSuccess()) {
        details += ", Reason=" + Runtime::ToString(result.failureReason) +
                   ", Error=" + result.errorMessage;
    }
    RecordEvent("Dispatcher", "StepResult:" + result.stepId, details);
}

std::string ExecutionReporter::GenerateReport() const {
    std::string report = "=== RUNTIME EXECUTION VALIDATION REPORT ===\n";
    for (const auto& ev : m_events) {
        report += "[" + ev.timestamp + "] [" + ev.component + "] " + ev.eventName + " -> " + ev.details + "\n";
    }
    report += "========================================\n";
    return report;
}

void ExecutionReporter::Clear() {
    m_events.clear();
}

} // namespace Testing
} // namespace NetDiscovery
