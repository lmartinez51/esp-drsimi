/**
 * @file SchedulerValidator.cpp
 * @brief Implementation of SchedulerValidator (v6.0 Phase C.5).
 */

#include "plan/SchedulerValidator.h"

namespace NetDiscovery {
namespace Plan {

ValidationReport SchedulerValidator::ValidateSchedulerState(ExecutionScheduler& scheduler, const ExecutionPlanInstance& instance) const {
    ValidationReport report;

    auto plan = instance.GetPlan();
    if (!plan || !plan->GetGraph()) {
        report.AddIssue(ValidationSeverity::Fatal, ValidationCode::NullGraph, "Scheduler", "Scheduler validation target has null plan or graph.");
        return report;
    }

    auto runnable = scheduler.GetNextRunnableNodes(instance);
    for (const auto& node : runnable) {
        if (!node) continue;

        std::string nodeId = node->GetNodeId();
        if (!scheduler.AreDependenciesSatisfied(instance, nodeId)) {
            report.AddIssue(
                ValidationSeverity::Error,
                ValidationCode::SchedulerInvariantViolation,
                nodeId,
                "Scheduler invariant violated: node returned as runnable but dependencies are unsatisfied: " + nodeId
            );
        }
    }

    return report;
}

} // namespace Plan
} // namespace NetDiscovery
