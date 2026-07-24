/**
 * @file ExecutionPlanner.h
 * @brief Thin orchestrator delegating plan construction to PlanBuilder and validation to PlanValidator (v5.0.0 Architecture Phase 8.6).
 */

#pragma once

#include "execution/InvocationRequest.h"
#include "execution/ExecutionContext.h"
#include "execution/ExecutionPolicy.h"
#include "execution/ExecutionCapabilities.h"
#include "execution/ExecutionPlanningResult.h"
#include "execution/PlanBuilder.h"
#include "execution/PlanValidator.h"
#include "binding/ActionBinding.h"
#include "core/StorageEventBus.h"

#include <vector>

namespace NetDiscovery {
namespace Execution {

/**
 * @brief Thin orchestration component executing plan creation through PlanBuilder and PlanValidator.
 */
class ExecutionPlanner {
public:
    explicit ExecutionPlanner(StorageEventBus* eventBus = nullptr);
    ~ExecutionPlanner() = default;

    void SetEventBus(StorageEventBus* eventBus);

    /**
     * @brief Creates a single-step ExecutionPlan, runs PlanValidator, and returns an ExecutionPlanningResult.
     */
    ExecutionPlanningResult CreatePlan(const InvocationRequest& request,
                                           const Binding::ActionBinding& selectedBinding,
                                           const ExecutionContext& context = {},
                                           const ExecutionPolicy& policy = {},
                                           const ExecutionCapabilities& capabilities = {}) const;

    /**
     * @brief Creates a multi-step composite ExecutionPlan, runs PlanValidator, and returns an ExecutionPlanningResult.
     */
    ExecutionPlanningResult CreateCompositePlan(const InvocationRequest& request,
                                                 const std::vector<Binding::ActionBinding>& selectedBindings,
                                                 const ExecutionContext& context = {},
                                                 const ExecutionPolicy& policy = {},
                                                 const ExecutionCapabilities& capabilities = {}) const;

    /**
     * @brief Delegates plan validation directly to PlanValidator.
     */
    PlanValidationResult ValidatePlan(const ExecutionPlan& plan) const;

private:
    void PublishPlanningEvent(StorageEventType type, const ExecutionPlan& plan, const PlanValidationResult* valResult = nullptr) const;

    StorageEventBus* m_eventBus{nullptr};
    PlanBuilder m_builder;
    PlanValidator m_validator;
};

} // namespace Execution
} // namespace NetDiscovery
