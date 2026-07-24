/**
 * @file ExecutionScheduler.h
 * @brief Step scheduling subsystem determining executable steps based on DAG progress (v5.0.0 Architecture Phase 9).
 */

#pragma once

#include "execution/ExecutionPlan.h"
#include "execution/ExecutionCursor.h"

#include <vector>

namespace NetDiscovery {
namespace Execution {

/**
 * @brief Scheduler subsystem deciding which steps in an ExecutionPlan graph are eligible for dispatch.
 */
class ExecutionScheduler {
public:
    ExecutionScheduler() = default;
    ~ExecutionScheduler() = default;

    /**
     * @brief Evaluates the ExecutionPlan and ExecutionCursor to return ready steps.
     */
    std::vector<ExecutionStep> GetReadySteps(const ExecutionPlan& plan, const ExecutionCursor& cursor) const;
};

} // namespace Execution
} // namespace NetDiscovery
