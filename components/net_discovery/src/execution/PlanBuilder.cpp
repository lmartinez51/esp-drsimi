/**
 * @file PlanBuilder.cpp
 * @brief Implementation of PlanBuilder subsystem (v5.0.0 Architecture Phase 8.6).
 */

#include "execution/PlanBuilder.h"
#include "esp_timer.h"

#include <unordered_set>
#include <algorithm>

namespace NetDiscovery {
namespace Execution {

PlanStatistics PlanBuilder::ComputeStatistics(const std::vector<ExecutionStep>& steps, const DependencyGraph& graph) const {
    PlanStatistics stats;
    std::unordered_set<std::string> uniqueAdapters;
    std::unordered_set<int> uniqueParallelGroups;

    for (const auto& step : steps) {
        stats.estimatedDurationMs += step.GetEstimatedDurationMs();
        stats.estimatedNetworkOperations++;
        uniqueAdapters.insert(step.GetAdapterId());
        
        if (step.GetParallelGroup() > 0) {
            uniqueParallelGroups.insert(step.GetParallelGroup());
        }
        if (step.GetRollbackStepId().has_value()) {
            stats.rollbackSteps++;
        }
        if (step.IsOptional()) {
            stats.optionalSteps++;
        }
    }

    stats.estimatedProtocolTransitions = static_cast<uint32_t>(uniqueAdapters.size());
    stats.parallelGroups = static_cast<uint32_t>(uniqueParallelGroups.size());

    // Calculate critical path and maximum depth from topological ordering
    std::vector<StepId> allStepIds;
    allStepIds.reserve(steps.size());
    for (const auto& step : steps) {
        allStepIds.push_back(step.GetStepId());
    }

    auto roots = graph.GetRootSteps(allStepIds);
    stats.maximumDepth = roots.empty() ? 0 : static_cast<uint32_t>(steps.size());
    stats.criticalPathLength = static_cast<uint32_t>(allStepIds.size());

    return stats;
}

std::pair<ExecutionPlan, ExecutionTrace> PlanBuilder::BuildSingleStepPlan(const InvocationRequest& request,
                                                                          const Binding::ActionBinding& selectedBinding,
                                                                          const ExecutionContext& context,
                                                                          const ExecutionPolicy& policy,
                                                                          const ExecutionCapabilities& capabilities) const {
    uint64_t timestampMs = static_cast<uint64_t>(esp_timer_get_time() / 1000);
    PlanIdentifier identifier(request.GetRequestId(), 1, timestampMs, 1);
    TraceId traceId = "trace." + identifier.ToString();

    ExecutionTrace trace(traceId, request.GetRequestId(), identifier.ToString(), timestampMs);
    trace.AddDecision("Building single-step plan for request '" + request.GetRequestId() + "' targeting entity '" + request.GetTargetEntityId() + "'");
    trace.AddDecision("ActionBinding selected: '" + selectedBinding.GetBindingId() + "' (Adapter: '" + selectedBinding.GetAdapterId() + "')");

    StepId stepId = identifier.ToString() + ".step.1";

    std::unordered_map<std::string, std::string> resolvedParameters;
    for (const auto& pBinding : selectedBinding.GetParameterBindings()) {
        auto it = request.GetParameters().find(pBinding.semanticParameter);
        if (it != request.GetParameters().end()) {
            resolvedParameters[pBinding.protocolParameter] = it->second;
        } else if (!pBinding.defaultValue.empty()) {
            resolvedParameters[pBinding.protocolParameter] = pBinding.defaultValue;
        }
    }
    for (const auto& [k, v] : request.GetParameters()) {
        if (resolvedParameters.find(k) == resolvedParameters.end()) {
            resolvedParameters[k] = v;
        }
    }

    ExecutionStep step(stepId,
                      selectedBinding.GetBindingId(),
                      selectedBinding.GetAdapterId(),
                      selectedBinding.GetOperationId(),
                      resolvedParameters,
                      selectedBinding.GetExecutionHints().estimatedDurationMs,
                      selectedBinding.GetExecutionHints().timeoutMs,
                      std::nullopt,
                      false,
                      0,
                      {{"ExecutionMode", ToString(policy.mode)}});

    std::vector<ExecutionStep> steps = {step};
    DependencyGraph depGraph;
    PlanStatistics stats = ComputeStatistics(steps, depGraph);

    ExecutionPlan plan(identifier,
                      std::move(steps),
                      std::move(depGraph),
                      std::move(stats),
                      capabilities,
                      policy,
                      PlanStatus::Pending,
                      {{"CallerId", request.GetCallerId()}, {"User", context.currentUser}});

    trace.AddDecision("ExecutionPlan built successfully: " + identifier.ToString() + " (Duration: " + std::to_string(plan.GetEstimatedDurationMs()) + "ms)");

    return {plan, trace};
}

std::pair<ExecutionPlan, ExecutionTrace> PlanBuilder::BuildCompositePlan(const InvocationRequest& request,
                                                                         const std::vector<Binding::ActionBinding>& selectedBindings,
                                                                         const ExecutionContext& context,
                                                                         const ExecutionPolicy& policy,
                                                                         const ExecutionCapabilities& capabilities) const {
    uint64_t timestampMs = static_cast<uint64_t>(esp_timer_get_time() / 1000);
    PlanIdentifier identifier(request.GetRequestId(), 1, timestampMs, 1);
    TraceId traceId = "trace." + identifier.ToString();

    ExecutionTrace trace(traceId, request.GetRequestId(), identifier.ToString(), timestampMs);
    trace.AddDecision("Building composite plan for request '" + request.GetRequestId() + "' across " + std::to_string(selectedBindings.size()) + " bindings");

    std::vector<ExecutionStep> steps;
    std::unordered_map<StepId, std::vector<StepId>> rawDeps;

    for (size_t i = 0; i < selectedBindings.size(); ++i) {
        const auto& binding = selectedBindings[i];
        StepId stepId = identifier.ToString() + ".step." + std::to_string(i + 1);

        std::unordered_map<std::string, std::string> resolvedParameters;
        for (const auto& pBinding : binding.GetParameterBindings()) {
            auto it = request.GetParameters().find(pBinding.semanticParameter);
            if (it != request.GetParameters().end()) {
                resolvedParameters[pBinding.protocolParameter] = it->second;
            } else if (!pBinding.defaultValue.empty()) {
                resolvedParameters[pBinding.protocolParameter] = pBinding.defaultValue;
            }
        }

        if (i > 0 && (!policy.allowParallel || !capabilities.supportsParallel)) {
            StepId prevStepId = identifier.ToString() + ".step." + std::to_string(i);
            rawDeps[stepId].push_back(prevStepId);
        }

        ExecutionStep step(stepId,
                          binding.GetBindingId(),
                          binding.GetAdapterId(),
                          binding.GetOperationId(),
                          resolvedParameters,
                          binding.GetExecutionHints().estimatedDurationMs,
                          binding.GetExecutionHints().timeoutMs,
                          std::nullopt,
                          false,
                          capabilities.supportsParallel && policy.allowParallel ? 1 : 0,
                          {{"SequenceIndex", std::to_string(i)}});

        steps.push_back(std::move(step));
    }

    DependencyGraph depGraph(rawDeps);
    PlanStatistics stats = ComputeStatistics(steps, depGraph);

    ExecutionPlan plan(identifier,
                      std::move(steps),
                      std::move(depGraph),
                      std::move(stats),
                      capabilities,
                      policy,
                      PlanStatus::Pending,
                      {{"Composite", "true"}, {"StepCount", std::to_string(selectedBindings.size())}});

    trace.AddDecision("Composite ExecutionPlan built successfully: " + identifier.ToString() + " with " + std::to_string(plan.GetStepCount()) + " steps");

    return {plan, trace};
}

} // namespace Execution
} // namespace NetDiscovery
