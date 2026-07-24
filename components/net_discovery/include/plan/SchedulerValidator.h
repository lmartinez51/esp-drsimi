/**
 * @file SchedulerValidator.h
 * @brief Validates scheduler invariants (deterministic ordering, dependency correctness, starvation prevention) (v6.0 Phase C.5).
 */

#pragma once

#include "plan/ExecutionScheduler.h"
#include "plan/ValidationReport.h"

namespace NetDiscovery {
namespace Plan {

class SchedulerValidator {
public:
    SchedulerValidator() = default;

    /**
     * @brief Validates scheduler invariants on a target plan instance state.
     */
    ValidationReport ValidateSchedulerState(ExecutionScheduler& scheduler, const ExecutionPlanInstance& instance) const;
};

} // namespace Plan
} // namespace NetDiscovery
