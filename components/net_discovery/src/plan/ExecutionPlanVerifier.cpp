/**
 * @file ExecutionPlanVerifier.cpp
 * @brief Implementation of ExecutionPlanVerifier (v6.0 Phase C.5).
 */

#include "plan/ExecutionPlanVerifier.h"
#include "esp_log.h"

static const char* TAG = "ExecutionPlanVerifier";

namespace NetDiscovery {
namespace Plan {

ValidationReport ExecutionPlanVerifier::VerifyPlan(const ExecutionPlanInstance& instance) const {
    ValidationReport report;

    ESP_LOGI(TAG, ">>> VerifyPlan: Step 1 Runtime Validation starting...");
    // 1. Runtime instance validation
    ValidationReport runtimeReport = m_runtimeValidator.ValidateInstance(instance);
    for (const auto& issue : runtimeReport.GetIssues()) {
        report.AddIssue(issue);
    }

    // Stop early if instance is null/fatal
    if (runtimeReport.HasErrors()) {
        ESP_LOGE(TAG, ">>> VerifyPlan: Step 1 Runtime Validation HAS ERRORS");
        return report;
    }

    ESP_LOGI(TAG, ">>> VerifyPlan: Step 2 Static Graph Validation starting...");
    // 2. Static graph validation
    auto plan = instance.GetPlan();
    if (plan && plan->GetGraph()) {
        ValidationReport staticReport = m_staticValidator.ValidateGraph(*plan->GetGraph());
        for (const auto& issue : staticReport.GetIssues()) {
            report.AddIssue(issue);
        }
    }

    ESP_LOGI(TAG, ">>> VerifyPlan: COMPLETED successfully");
    return report;
}

} // namespace Plan
} // namespace NetDiscovery