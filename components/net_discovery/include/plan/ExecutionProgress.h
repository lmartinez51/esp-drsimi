/**
 * @file ExecutionProgress.h
 * @brief Aggregated execution progress metrics (v6.0 Phase C).
 */

#pragma once

#include "plan/ExecutionState.h"
#include <string>
#include <cstdint>

namespace NetDiscovery {
namespace Plan {

struct ExecutionProgress {
    std::string stepId;
    std::string stepName;
    int completedSteps{0};
    int totalSteps{0};
    float percentage{0.0f};
    uint64_t elapsedTimeMs{0};
    uint64_t estimatedRemainingMs{0};
    PlanState state{PlanState::Pending};
};

} // namespace Plan
} // namespace NetDiscovery
