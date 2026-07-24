/**
 * @file ExecutionPlannerTypes.h
 * @brief Common types, aliases, and enums for Execution Planning Engine (v5.0.0 Architecture Phase 8.5).
 */

#pragma once

#include <string>
#include <cstdint>

namespace NetDiscovery {
namespace Execution {

using RequestId = std::string;
using PlanId = std::string;
using StepId = std::string;
using TraceId = std::string;

/**
 * @brief Execution plan lifecycle status.
 */
enum class PlanStatus {
    Pending,           // Created, waiting for validation or queueing
    Validated,         // Schema & structural dependency check passed
    ValidationFailed,  // Missing dependencies or invalid bindings
    Cancelled,         // Pre-execution cancellation request
    Executing,         // [Future Phase 9] Active execution
    Success,           // [Future Phase 9] All steps completed successfully
    Failed,            // [Future Phase 9] Execution failed
    RolledBack         // [Future Phase 9] Rollback steps executed
};

/**
 * @brief String conversion helper for PlanStatus.
 */
inline std::string ToString(PlanStatus status) {
    switch (status) {
        case PlanStatus::Pending:          return "Pending";
        case PlanStatus::Validated:        return "Validated";
        case PlanStatus::ValidationFailed: return "ValidationFailed";
        case PlanStatus::Cancelled:        return "Cancelled";
        case PlanStatus::Executing:        return "Executing";
        case PlanStatus::Success:          return "Success";
        case PlanStatus::Failed:           return "Failed";
        case PlanStatus::RolledBack:       return "RolledBack";
        default:                           return "Unknown";
    }
}

} // namespace Execution
} // namespace NetDiscovery
