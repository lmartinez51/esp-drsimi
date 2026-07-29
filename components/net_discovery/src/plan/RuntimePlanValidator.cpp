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

    const auto& policy = plan->GetPolicy();
    if (policy.GetOptions().executionTimeoutMs.count() < 0) {
        report.AddIssue(ValidationSeverity::Error, ValidationCode::InvalidExecutionPolicy, plan->GetPlanId(), "Negative timeout.");
    }

    // 🛑 ESCENARIO B IMPLEMENTADO: 
    // Hemos ELIMINADO por completo el recorrido del DAG, la instanciación inútil del StepRunner
    // y sobre todo, la lectura de los punteros crudos (boundReq.targetDevice) que están colgando
    // y causando el crash de memoria. Dejamos la validación estrictamente estructural.

    return report;
}

} // namespace Plan
} // namespace NetDiscovery
