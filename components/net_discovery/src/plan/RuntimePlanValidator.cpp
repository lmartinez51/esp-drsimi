/**
 * @file RuntimePlanValidator.cpp
 * @brief Implementation of RuntimePlanValidator (v6.0 Phase C.5).
 */

#include "plan/RuntimePlanValidator.h"
#include "plan/ActionStep.h"
#include "plan/StepRunnerFactory.h"

namespace NetDiscovery {
namespace Plan {

ValidationReport RuntimePlanValidator::ValidateInstance(const ExecutionPlanInstance& instance) const {
    ValidationReport report;

    auto plan = instance.GetPlan();
    if (!plan) {
        report.AddIssue(ValidationSeverity::Fatal, ValidationCode::NullPlan, "Instance", "ExecutionPlanInstance carries a null plan pointer.");
        return report;
    }

    auto graph = plan->GetGraph();
    if (!graph) {
        report.AddIssue(ValidationSeverity::Fatal, ValidationCode::NullGraph, "Plan", "IExecutionPlan carries a null graph pointer.");
        return report;
    }

    // Validate execution policy profile
    const auto& policy = plan->GetPolicy();
    if (policy.GetOptions().executionTimeoutMs.count() < 0) {
        report.AddIssue(ValidationSeverity::Error, ValidationCode::InvalidExecutionPolicy, plan->GetPlanId(), "Plan policy carries negative timeout.");
    }

    auto nodes = graph->GetNodes();
    for (const auto& node : nodes) {
        if (!node) continue;

        auto step = node->GetStep();
        if (!step) {
            report.AddIssue(ValidationSeverity::Error, ValidationCode::NullStep, node->GetNodeId(), "Node carries null step pointer.");
            continue;
        }

        // Validate StepRunner resolution
        auto runner = StepRunnerFactory::CreateRunner(*step);
        if (!runner) {
            report.AddIssue(ValidationSeverity::Error, ValidationCode::MissingStepRunner, step->GetStepId(), "Failed to resolve StepRunner for step: " + step->GetStepId());
        }

        // Validate ActionStep bindings
        if (step->GetStepType() == StepType::Action) {
            auto* actionStep = static_cast<ActionStep*>(step.get());
            const auto& boundReq = actionStep->GetBoundRequest();

            if (!boundReq.targetDevice) {
                report.AddIssue(ValidationSeverity::Error, ValidationCode::InvalidBoundRequest, step->GetStepId(), "ActionStep missing targetDevice pointer.");
            }
            if (!boundReq.selectedController) {
                report.AddIssue(ValidationSeverity::Error, ValidationCode::MissingController, step->GetStepId(), "ActionStep missing selectedController pointer.");
            }
        }
    }

    return report;
}

} // namespace Plan
} // namespace NetDiscovery
