/**
 * @file ValidationIssue.h
 * @brief Represents a single validation issue detected during static or runtime plan verification (v6.0 Phase C.5).
 */

#pragma once

#include <string>

namespace NetDiscovery {
namespace Plan {

enum class ValidationSeverity {
    Warning,
    Error,
    Fatal
};

enum class ValidationCode {
    None,
    NullPlan,
    NullGraph,
    EmptyGraph,
    DuplicateNodeId,
    DependencyCycle,
    OrphanNode,
    InvalidEdgeReference,
    UnreachableNode,
    MissingEntryNode,
    NullStep,
    MissingStepRunner,
    InvalidExecutionPolicy,
    InvalidBoundRequest,
    MissingController,
    MissingTransport,
    InvalidRollbackConfig,
    SchedulerInvariantViolation
};

struct ValidationIssue {
    ValidationSeverity severity{ValidationSeverity::Error};
    ValidationCode code{ValidationCode::None};
    std::string componentId;
    std::string message;

    std::string ToString() const {
        std::string sevStr = (severity == ValidationSeverity::Fatal) ? "[FATAL]" :
                             (severity == ValidationSeverity::Error) ? "[ERROR]" : "[WARN]";
        return sevStr + " [" + componentId + "] " + message;
    }
};

} // namespace Plan
} // namespace NetDiscovery
