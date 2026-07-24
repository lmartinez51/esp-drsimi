/**
 * @file ExecutionResult.h
 * @brief Future-compatible execution result model (v5.0.0 Architecture Phase 8.5).
 * 
 * Defined for future Phase 9 execution engine compatibility. Contains status, completed 
 * steps, failed steps, duration, and error maps.
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
 * @brief Future-compatible result structure returned by future execution engines.
 */
struct ExecutionResult {
    PlanId planId;                                                   // Associated plan ID
    RequestId requestId;                                             // Associated invocation request ID
    PlanStatus status{PlanStatus::Pending};                          // Outcome status
    std::vector<StepId> completedStepIds;                            // List of successfully completed step IDs
    std::vector<StepId> failedStepIds;                               // List of failed step IDs
    uint32_t durationMs{0};                                          // Actual total execution duration in ms
    std::unordered_map<StepId, std::string> errors;                 // Error messages mapped by StepId
    std::unordered_map<std::string, std::string> metadata;          // Extensible result metadata

    ExecutionResult() = default;

    ExecutionResult(PlanId pId, RequestId rId, PlanStatus st)
        : planId(std::move(pId)), requestId(std::move(rId)), status(st) {}
};

} // namespace Execution
} // namespace NetDiscovery
