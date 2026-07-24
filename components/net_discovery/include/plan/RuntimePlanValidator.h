/**
 * @file RuntimePlanValidator.h
 * @brief Validates runtime resource bindings, controllers, and policy configurations (v6.0 Phase C.5).
 */

#pragma once

#include "plan/ExecutionPlanInstance.h"
#include "plan/ValidationReport.h"

namespace NetDiscovery {
namespace Plan {

class RuntimePlanValidator {
public:
    RuntimePlanValidator() = default;

    /**
     * @brief Performs single-pass validation of an ExecutionPlanInstance before execution.
     */
    ValidationReport ValidateInstance(const ExecutionPlanInstance& instance) const;
};

} // namespace Plan
} // namespace NetDiscovery
