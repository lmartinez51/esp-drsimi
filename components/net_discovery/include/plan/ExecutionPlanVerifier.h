/**
 * @file ExecutionPlanVerifier.h
 * @brief Unified verifier coordinating StaticPlanValidator and RuntimePlanValidator (v6.0 Phase C.5).
 */

#pragma once

#include "plan/StaticPlanValidator.h"
#include "plan/RuntimePlanValidator.h"
#include "plan/ExecutionPlanInstance.h"

namespace NetDiscovery {
namespace Plan {

class ExecutionPlanVerifier {
public:
    ExecutionPlanVerifier() = default;

    /**
     * @brief Performs complete pre-flight verification (static DAG + runtime resources).
     */
    ValidationReport VerifyPlan(const ExecutionPlanInstance& instance) const;

private:
    StaticPlanValidator m_staticValidator;
    RuntimePlanValidator m_runtimeValidator;
};

} // namespace Plan
} // namespace NetDiscovery
