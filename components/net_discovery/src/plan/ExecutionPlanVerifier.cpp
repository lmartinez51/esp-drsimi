/**
 * @file ExecutionPlanVerifier.cpp
 * @brief Implementation of ExecutionPlanVerifier (v6.0 Phase C.5).
 */

#include "plan/ExecutionPlanVerifier.h"

namespace NetDiscovery {
namespace Plan {

ValidationReport ExecutionPlanVerifier::VerifyPlan(const ExecutionPlanInstance& instance) const {
    ValidationReport report;

    // 1. Runtime instance validation
    ValidationReport runtimeReport = m_runtimeValidator.ValidateInstance(instance);
    for (const auto& issue : runtimeReport.GetIssues()) {
        report.AddIssue(issue);
    }

    // Stop early if instance is null/fatal
    if (runtimeReport.HasErrors()) {
        return report;
    }

    // 2. Static graph validation
    auto plan = instance.GetPlan();
    if (plan && plan->GetGraph()) {
        ValidationReport staticReport = m_staticValidator.ValidateGraph(*plan->GetGraph());
        for (const auto& issue : staticReport.GetIssues()) {
            report.AddIssue(issue);
        }
    }

    return report;
}

} // namespace Plan
} // namespace NetDiscovery
