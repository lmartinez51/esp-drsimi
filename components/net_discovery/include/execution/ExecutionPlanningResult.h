/**
 * @file ExecutionPlanningResult.h
 * @brief Complete outcome structure returned by ExecutionPlanner (v5.0.0 Architecture Phase 8.6).
 */

#pragma once

#include "execution/ExecutionPlan.h"
#include "execution/ExecutionTrace.h"
#include "execution/PlanValidationResult.h"

namespace NetDiscovery {
namespace Execution {

/**
 * @brief Result model containing the constructed ExecutionPlan, ExecutionTrace, and PlanValidationResult.
 */
struct ExecutionPlanningResult {
    ExecutionPlan plan;                           // Constructed execution plan
    ExecutionTrace trace;                         // Diagnostic decision trace
    PlanValidationResult validationResult;         // Plan validation status and diagnostics

    ExecutionPlanningResult() = default;

    ExecutionPlanningResult(ExecutionPlan p, ExecutionTrace t, PlanValidationResult v)
        : plan(std::move(p)), trace(std::move(t)), validationResult(std::move(v)) {}

    bool IsValid() const {
        return validationResult.valid;
    }
};

} // namespace Execution
} // namespace NetDiscovery
