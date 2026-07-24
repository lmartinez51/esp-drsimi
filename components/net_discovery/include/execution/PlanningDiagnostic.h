/**
 * @file PlanningDiagnostic.h
 * @brief Planning diagnostic message structure (v5.0.0 Architecture Phase 8.6).
 */

#pragma once

#include "execution/ExecutionPlannerTypes.h"

#include <string>

namespace NetDiscovery {
namespace Execution {

enum class DiagnosticSeverity {
    Info,
    Warning,
    Error
};

inline std::string ToString(DiagnosticSeverity severity) {
    switch (severity) {
        case DiagnosticSeverity::Info:    return "Info";
        case DiagnosticSeverity::Warning: return "Warning";
        case DiagnosticSeverity::Error:   return "Error";
        default:                          return "Unknown";
    }
}

/**
 * @brief Planning diagnostic entry capturing issues, severity, and suggested remedies.
 */
struct PlanningDiagnostic {
    DiagnosticSeverity severity{DiagnosticSeverity::Info};
    std::string source;       // Origin component e.g. "PlanValidator.CycleCheck"
    StepId stepId;            // StepId related to diagnostic (or empty)
    std::string message;      // Descriptive diagnostic explanation
    std::string suggestion;   // Suggested fix or remedy

    PlanningDiagnostic() = default;

    PlanningDiagnostic(DiagnosticSeverity sev, std::string src, StepId sId, std::string msg, std::string sug = "")
        : severity(sev), source(std::move(src)), stepId(std::move(sId)), message(std::move(msg)), suggestion(std::move(sug)) {}
};

} // namespace Execution
} // namespace NetDiscovery
