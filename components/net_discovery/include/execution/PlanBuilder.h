/**
 * @file PlanBuilder.h
 * @brief Subsystem building ExecutionSteps, DependencyGraph, PlanStatistics, and ExecutionTrace (v5.0.0 Architecture Phase 8.6).
 */

#pragma once

#include "execution/InvocationRequest.h"
#include "execution/ExecutionContext.h"
#include "execution/ExecutionStep.h"
#include "execution/ExecutionPolicy.h"
#include "execution/ExecutionCapabilities.h"
#include "execution/ExecutionPlan.h"
#include "execution/ExecutionTrace.h"
#include "binding/ActionBinding.h"

#include <vector>
#include <utility>

namespace NetDiscovery {
namespace Execution {

/**
 * @brief Builder component constructing execution steps, dependency graphs, and plan statistics.
 */
class PlanBuilder {
public:
    PlanBuilder() = default;
    ~PlanBuilder() = default;

    /**
     * @brief Builds a single-step ExecutionPlan and diagnostic trace.
     */
    std::pair<ExecutionPlan, ExecutionTrace> BuildSingleStepPlan(const InvocationRequest& request,
                                                                 const Binding::ActionBinding& selectedBinding,
                                                                 const ExecutionContext& context,
                                                                 const ExecutionPolicy& policy,
                                                                 const ExecutionCapabilities& capabilities) const;

    /**
     * @brief Builds a multi-step composite ExecutionPlan graph and trace.
     */
    std::pair<ExecutionPlan, ExecutionTrace> BuildCompositePlan(const InvocationRequest& request,
                                                                const std::vector<Binding::ActionBinding>& selectedBindings,
                                                                const ExecutionContext& context,
                                                                const ExecutionPolicy& policy,
                                                                const ExecutionCapabilities& capabilities) const;

private:
    PlanStatistics ComputeStatistics(const std::vector<ExecutionStep>& steps, const DependencyGraph& graph) const;
};

} // namespace Execution
} // namespace NetDiscovery
