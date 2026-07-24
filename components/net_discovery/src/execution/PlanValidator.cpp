/**
 * @file PlanValidator.cpp
 * @brief Implementation of PlanValidator subsystem (v5.0.0 Architecture Phase 8.6).
 */

#include "execution/PlanValidator.h"

#include <unordered_set>
#include <unordered_map>

namespace NetDiscovery {
namespace Execution {

PlanValidationResult PlanValidator::Validate(const ExecutionPlan& plan) const {
    PlanValidationResult result;

    // 1. Basic Plan Identifier & Empty Check
    if (plan.GetPlanId().empty()) {
        result.AddDiagnostic({DiagnosticSeverity::Error, "PlanValidator.IdentifierCheck", "", "PlanIdentifier canonical string is empty", "Ensure RequestId is provided."});
    }
    if (plan.GetRequestId().empty()) {
        result.AddDiagnostic({DiagnosticSeverity::Error, "PlanValidator.IdentifierCheck", "", "RequestId is empty", "Provide a valid RequestId."});
    }
    if (plan.IsEmpty()) {
        result.AddDiagnostic({DiagnosticSeverity::Error, "PlanValidator.EmptyCheck", "", "ExecutionPlan contains zero steps", "Add at least one ExecutionStep."});
        return result;
    }

    // Collect all step IDs and detect duplicate StepIds or missing bindings
    std::unordered_set<StepId> knownStepIds;
    std::vector<StepId> allStepIds;
    allStepIds.reserve(plan.GetStepCount());

    for (const auto& step : plan.GetSteps()) {
        if (step.GetStepId().empty()) {
            result.AddDiagnostic({DiagnosticSeverity::Error, "PlanValidator.StepCheck", "", "ExecutionStep has empty StepId", "Assign a unique step identifier."});
        }
        if (step.GetBindingId().empty()) {
            result.AddDiagnostic({DiagnosticSeverity::Error, "PlanValidator.StepCheck", step.GetStepId(), "ExecutionStep has empty BindingId", "Bind step to a valid ActionBinding."});
        }
        if (step.GetAdapterId().empty()) {
            result.AddDiagnostic({DiagnosticSeverity::Error, "PlanValidator.StepCheck", step.GetStepId(), "ExecutionStep has empty AdapterId", "Specify a target protocol adapter."});
        }

        if (knownStepIds.find(step.GetStepId()) != knownStepIds.end()) {
            result.AddDiagnostic({DiagnosticSeverity::Error, "PlanValidator.DuplicateStepCheck", step.GetStepId(), "Duplicate StepId detected: " + step.GetStepId(), "Ensure all StepIds are unique."});
        } else {
            knownStepIds.insert(step.GetStepId());
            allStepIds.push_back(step.GetStepId());
        }

        // Timeout check
        if (step.GetTimeoutMs() == 0) {
            result.AddDiagnostic({DiagnosticSeverity::Warning, "PlanValidator.TimeoutCheck", step.GetStepId(), "Step timeout is 0ms", "Set a reasonable timeoutMs."});
        } else if (step.GetTimeoutMs() > plan.GetExecutionPolicy().overallTimeoutMs) {
            result.AddDiagnostic({DiagnosticSeverity::Warning, "PlanValidator.TimeoutCheck", step.GetStepId(), "Step timeout exceeds overall plan timeout", "Increase overallTimeoutMs or reduce step timeout."});
        }

        // Rollback step reference check
        if (step.GetRollbackStepId().has_value()) {
            const auto& rollbackId = step.GetRollbackStepId().value();
            if (knownStepIds.find(rollbackId) == knownStepIds.end()) {
                // Will double check in secondary pass if rollback is defined after step
            }
        }
    }

    // 2. Validate Rollback References
    for (const auto& step : plan.GetSteps()) {
        if (step.GetRollbackStepId().has_value()) {
            const auto& rollbackId = step.GetRollbackStepId().value();
            if (knownStepIds.find(rollbackId) == knownStepIds.end()) {
                result.AddDiagnostic({DiagnosticSeverity::Error, "PlanValidator.RollbackCheck", step.GetStepId(), "Step references invalid/non-existent rollbackStepId: " + rollbackId, "Ensure rollback step exists in plan."});
            }
        }
    }

    // 3. Dependency Cycle Check via DependencyGraph
    const auto& depGraph = plan.GetDependencyGraph();
    if (depGraph.HasCycle(allStepIds)) {
        result.AddDiagnostic({DiagnosticSeverity::Error, "PlanValidator.CycleCheck", "", "Dependency cycle detected in ExecutionPlan DAG graph", "Remove circular dependencies."});
    }

    // 4. Orphan Dependency Target Check
    for (const auto& [stepId, depList] : depGraph.GetRawDependencies()) {
        if (knownStepIds.find(stepId) == knownStepIds.end()) {
            result.AddDiagnostic({DiagnosticSeverity::Error, "PlanValidator.OrphanCheck", stepId, "Dependency graph references unknown target stepId: " + stepId, "Clean up invalid dependency keys."});
        }
        for (const auto& depId : depList) {
            if (knownStepIds.find(depId) == knownStepIds.end()) {
                result.AddDiagnostic({DiagnosticSeverity::Error, "PlanValidator.OrphanCheck", stepId, "Step " + stepId + " depends on non-existent stepId: " + depId, "Verify dependency step IDs."});
            }
        }
    }

    // 5. ExecutionPolicy vs Platform Capabilities Consistency Check
    const auto& policy = plan.GetExecutionPolicy();
    const auto& caps = plan.GetCapabilities();

    if (policy.allowParallel && !caps.supportsParallel) {
        result.AddDiagnostic({DiagnosticSeverity::Warning, "PlanValidator.CapabilityCheck", "", "ExecutionPolicy requested parallel scheduling, but platform capabilities disable it", "Parallel steps will execute sequentially."});
    }
    if (policy.mode == ExecutionMode::RollbackOnFailure && !caps.supportsRollback) {
        result.AddDiagnostic({DiagnosticSeverity::Error, "PlanValidator.CapabilityCheck", "", "ExecutionPolicy specifies RollbackOnFailure, but platform capabilities disable rollback", "Enable supportsRollback or change execution mode."});
    }

    if (result.valid) {
        result.AddDiagnostic({DiagnosticSeverity::Info, "PlanValidator.Success", "", "ExecutionPlan validation passed cleanly with 0 errors.", ""});
    }

    return result;
}

} // namespace Execution
} // namespace NetDiscovery
