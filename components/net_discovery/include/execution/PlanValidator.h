/**
 * @file PlanValidator.h
 * @brief Independent structural & topological validation subsystem for ExecutionPlans (v5.0.0 Architecture Phase 8.6).
 */

#pragma once

#include "execution/ExecutionPlan.h"
#include "execution/PlanValidationResult.h"

namespace NetDiscovery {
namespace Execution {

/**
 * @brief Dedicated validation subsystem checking execution graph consistency, cycle freedom, and policy compliance.
 */
class PlanValidator {
public:
    PlanValidator() = default;
    ~PlanValidator() = default;

    /**
     * @brief Performs comprehensive multi-stage validation of an ExecutionPlan.
     */
    PlanValidationResult Validate(const ExecutionPlan& plan) const;
};

} // namespace Execution
} // namespace NetDiscovery
