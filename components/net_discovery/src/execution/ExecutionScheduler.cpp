/**
 * @file ExecutionScheduler.cpp
 * @brief Implementation of ExecutionScheduler (v5.0.0 Architecture Phase 9).
 */

#include "execution/ExecutionScheduler.h"

#include <unordered_map>

namespace NetDiscovery {
namespace Execution {

std::vector<ExecutionStep> ExecutionScheduler::GetReadySteps(const ExecutionPlan& plan, const ExecutionCursor& cursor) const {
    std::vector<ExecutionStep> readySteps;
    std::vector<StepId> readyStepIds = cursor.GetReadySteps();

    if (readyStepIds.empty()) {
        return readySteps;
    }

    std::unordered_map<StepId, ExecutionStep> stepMap;
    for (const auto& step : plan.GetSteps()) {
        stepMap[step.GetStepId()] = step;
    }

    for (const auto& stepId : readyStepIds) {
        auto it = stepMap.find(stepId);
        if (it != stepMap.end()) {
            readySteps.push_back(it->second);
        }
    }

    return readySteps;
}

} // namespace Execution
} // namespace NetDiscovery
