// Target: components/net_discovery/src/compiler/PassThroughPlanOptimizer.cpp
// Objective: Add diagnostic logs before each optimization pass to isolate the exact function causing the deadlock.

#include "compiler/PassThroughPlanOptimizer.h"
#include "esp_log.h"

static const char* TAG = "PassThroughOptimizer";

namespace NetDiscovery {
namespace compiler {

std::shared_ptr<Plan::IExecutionPlan> PassThroughPlanOptimizer::Optimize(
    std::shared_ptr<Plan::IExecutionPlan> plan) const
{
    if (!plan) {
        ESP_LOGW(TAG, "Optimize: Received null plan");
        return plan;
    }

    ESP_LOGI(TAG, "Optimize: entering FoldConstants");
    auto result = FoldConstants(plan);
    
    ESP_LOGI(TAG, "Optimize: entering EliminateDeadBranches");
    result = EliminateDeadBranches(result);
    
    ESP_LOGI(TAG, "Optimize: entering EliminateDuplicateActions");
    result = EliminateDuplicateActions(result);
    
    ESP_LOGI(TAG, "Optimize: entering RemoveRedundantDelays");
    //result = RemoveRedundantDelays(result);
    
    ESP_LOGI(TAG, "Optimize: all passes completed");

    ESP_LOGD(TAG, "Optimize: plan='%s' nodes=%zu (pass-through)",
             result ? result->GetPlanId().c_str() : "null",
             result && result->GetGraph() ? result->GetGraph()->GetNodes().size() : 0u);

    return result;
}

} // namespace compiler
} // namespace NetDiscovery