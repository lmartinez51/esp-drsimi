/**
 * @file PassThroughPlanOptimizer.cpp
 * @brief Concrete pass-through IPlanOptimizer implementation.
 *
 * ESP-Claw Platform — Phase E (Intent Compiler & End-to-End Integration)
 */

#include "compiler/PassThroughPlanOptimizer.h"
#include "esp_log.h"

static const char* TAG = "PassThroughOptimizer";

namespace NetDiscovery {
namespace compiler {

std::shared_ptr<Plan::IExecutionPlan> PassThroughPlanOptimizer::Optimize(
    std::shared_ptr<Plan::IExecutionPlan> plan) const
{
    // Pass-through: return unchanged
    // Apply future override hooks (derived class extension points)
    auto result = FoldConstants(plan);
    result = EliminateDeadBranches(result);
    result = EliminateDuplicateActions(result);
    result = RemoveRedundantDelays(result);

    ESP_LOGD(TAG, "Optimize: plan='%s' nodes=%zu (pass-through)",
             result ? result->GetPlanId().c_str() : "null",
             result ? result->GetGraph()->GetNodes().size() : 0u);

    return result;
}

} // namespace compiler
} // namespace NetDiscovery
