/**
 * @file ExecutionPlanner.cpp
 * @brief Implementation of thin orchestrator ExecutionPlanner (v5.0.0 Architecture Phase 8.6).
 */

#include "execution/ExecutionPlanner.h"
#include "execution/ExecutionEvents.h"
#include "esp_timer.h"

namespace NetDiscovery {
namespace Execution {

ExecutionPlanner::ExecutionPlanner(StorageEventBus* eventBus)
    : m_eventBus(eventBus) {}

void ExecutionPlanner::SetEventBus(StorageEventBus* eventBus) {
    m_eventBus = eventBus;
}

ExecutionPlanningResult ExecutionPlanner::CreatePlan(const InvocationRequest& request,
                                                     const Binding::ActionBinding& selectedBinding,
                                                     const ExecutionContext& context,
                                                     const ExecutionPolicy& policy,
                                                     const ExecutionCapabilities& capabilities) const {
    // 1. Delegate plan and trace construction to PlanBuilder
    auto [plan, trace] = m_builder.BuildSingleStepPlan(request, selectedBinding, context, policy, capabilities);

    // 2. Delegate validation to PlanValidator
    PlanValidationResult valResult = m_validator.Validate(plan);

    // 3. Publish metadata events based on validation status
    PublishPlanningEvent(StorageEventType::ExecutionPlanCreated, plan, &valResult);
    if (valResult.valid) {
        PublishPlanningEvent(StorageEventType::ExecutionPlanValidated, plan, &valResult);
    } else {
        PublishPlanningEvent(StorageEventType::ExecutionPlanRejected, plan, &valResult);
    }

    return ExecutionPlanningResult(std::move(plan), std::move(trace), std::move(valResult));
}

ExecutionPlanningResult ExecutionPlanner::CreateCompositePlan(const InvocationRequest& request,
                                                           const std::vector<Binding::ActionBinding>& selectedBindings,
                                                           const ExecutionContext& context,
                                                           const ExecutionPolicy& policy,
                                                           const ExecutionCapabilities& capabilities) const {
    // 1. Delegate composite plan and trace construction to PlanBuilder
    auto [plan, trace] = m_builder.BuildCompositePlan(request, selectedBindings, context, policy, capabilities);

    // 2. Delegate validation to PlanValidator
    PlanValidationResult valResult = m_validator.Validate(plan);

    // 3. Publish metadata events
    PublishPlanningEvent(StorageEventType::ExecutionPlanCreated, plan, &valResult);
    if (valResult.valid) {
        PublishPlanningEvent(StorageEventType::ExecutionPlanValidated, plan, &valResult);
    } else {
        PublishPlanningEvent(StorageEventType::ExecutionPlanRejected, plan, &valResult);
    }

    return ExecutionPlanningResult(std::move(plan), std::move(trace), std::move(valResult));
}

PlanValidationResult ExecutionPlanner::ValidatePlan(const ExecutionPlan& plan) const {
    return m_validator.Validate(plan);
}

void ExecutionPlanner::PublishPlanningEvent(StorageEventType type, const ExecutionPlan& plan, const PlanValidationResult* valResult) const {
    if (!m_eventBus) return;

    StorageEvent event;
    event.type = type;
    event.entityId = plan.GetPlanId();
    event.timestamp = static_cast<uint64_t>(esp_timer_get_time() / 1000);
    event.metadata[ExecutionEventKeys::PlanId] = plan.GetPlanId();
    event.metadata[ExecutionEventKeys::RequestId] = plan.GetRequestId();
    event.metadata[ExecutionEventKeys::StepCount] = std::to_string(plan.GetStepCount());
    event.metadata[ExecutionEventKeys::EstimatedDurationMs] = std::to_string(plan.GetEstimatedDurationMs());
    event.metadata[ExecutionEventKeys::PolicyMode] = ToString(plan.GetExecutionPolicy().mode);

    if (valResult && !valResult->errors.empty()) {
        event.metadata[ExecutionEventKeys::ValidationFailureReason] = valResult->errors.front().message;
    }

    m_eventBus->Publish(event);
}

} // namespace Execution
} // namespace NetDiscovery
