/**
 * @file ExecutionPlanExecutor.cpp
 * @brief Implementation of ExecutionPlanExecutor with pre-flight verifier and clock abstraction (v6.0 Phase C.5).
 */

#include "plan/ExecutionPlanExecutor.h"
#include "plan/StepRunnerFactory.h"
#include "plan/binding/DefaultBindingResolver.h"
#include "esp_log.h"
#include <algorithm>

static const char* TAG = "ExecutionPlanExecutor";

namespace NetDiscovery {
namespace Plan {

ExecutionPlanExecutor::ExecutionPlanExecutor(std::shared_ptr<ExecutionInfrastructure> infrastructure,
                                               std::shared_ptr<ExecutionPlanVerifier> verifier,
                                               std::shared_ptr<IRuntimeClock> clock)
    : m_infrastructure(std::move(infrastructure))
    , m_verifier(std::move(verifier))
    , m_clock(std::move(clock))
{
    if (!m_verifier) {
        m_verifier = std::make_shared<ExecutionPlanVerifier>();
    }
    if (!m_clock) {
        m_clock = std::make_shared<DefaultRuntimeClock>();
    }
}

void ExecutionPlanExecutor::RegisterObserver(std::shared_ptr<IExecutionPlanObserver> observer) {
    if (observer) {
        m_observers.push_back(observer);
    }
}

void ExecutionPlanExecutor::UnregisterObserver(std::shared_ptr<IExecutionPlanObserver> observer) {
    m_observers.erase(std::remove(m_observers.begin(), m_observers.end(), observer), m_observers.end());
}

void ExecutionPlanExecutor::NotifyObservers(const ExecutionEvent& event) {
    for (const auto& obs : m_observers) {
        if (obs) obs->OnExecutionEvent(event);
    }
}

ExecutionResult ExecutionPlanExecutor::ExecutePlan(ExecutionPlanInstance& instance) {
    ExecutionResult finalResult;

    if (!m_infrastructure) {
        finalResult.status = ExecutionStatus::ExecutionFailed;
        finalResult.errorMessage = "Missing ExecutionInfrastructure in ExecutionPlanExecutor.";
        return finalResult;
    }

    // 1. Single-Pass Pre-Flight Plan Verification (Static DAG + Runtime Resources)
    ValidationReport verificationReport = m_verifier->VerifyPlan(instance);
    if (verificationReport.HasErrors()) {
        ESP_LOGE(TAG, "Pre-flight verification failed:\n%s", verificationReport.ToString().c_str());

        instance.SetState(PlanState::Failed);

        ExecutionEvent failEvent;
        failEvent.type = ExecutionEventType::PlanFailed;
        failEvent.instanceId = instance.GetInstanceId();
        failEvent.planId = instance.GetPlan() ? instance.GetPlan()->GetPlanId() : "";
        failEvent.timestampMs = m_clock->GetCurrentTimeMs();
        failEvent.planState = PlanState::Failed;
        failEvent.errorMessage = "Pre-flight verification failed:\n" + verificationReport.ToString();
        failEvent.progress = instance.GetProgress();
        NotifyObservers(failEvent);

        finalResult.status = ExecutionStatus::ExecutionFailed;
        finalResult.errorMessage = failEvent.errorMessage;
        return finalResult;
    }

    auto plan = instance.GetPlan();
    instance.SetState(PlanState::Running);

    ExecutionEvent startEvent;
    startEvent.type = ExecutionEventType::PlanStarted;
    startEvent.instanceId = instance.GetInstanceId();
    startEvent.planId = plan->GetPlanId();
    startEvent.timestampMs = m_clock->GetCurrentTimeMs();
    startEvent.planState = PlanState::Running;
    startEvent.progress = instance.GetProgress();
    NotifyObservers(startEvent);

    std::vector<std::shared_ptr<IExecutionStep>> executedSteps;

    // 2. Execution Loop
    while (true) {
        if (instance.GetCancellationToken().IsCancelled()) {
            instance.SetState(PlanState::Cancelled);

            ExecutionEvent cancelEvent;
            cancelEvent.type = ExecutionEventType::Cancelled;
            cancelEvent.instanceId = instance.GetInstanceId();
            cancelEvent.planId = plan->GetPlanId();
            cancelEvent.timestampMs = m_clock->GetCurrentTimeMs();
            cancelEvent.planState = PlanState::Cancelled;
            cancelEvent.progress = instance.GetProgress();
            NotifyObservers(cancelEvent);

            finalResult.status = ExecutionStatus::ExecutionFailed;
            finalResult.errorMessage = "ExecutionPlan cancelled.";
            return finalResult;
        }

        auto runnableNodes = m_scheduler.GetNextRunnableNodes(instance);
        if (runnableNodes.empty()) {
            break;
        }

        for (const auto& node : runnableNodes) {
            auto step = node->GetStep();
            if (!step) continue;

            std::string stepId = step->GetStepId();
            instance.SetStepState(stepId, StepState::Running);

            ExecutionEvent stepStart;
            stepStart.type = ExecutionEventType::StepStarted;
            stepStart.instanceId = instance.GetInstanceId();
            stepStart.planId = plan->GetPlanId();
            stepStart.stepId = stepId;
            stepStart.stepName = step->GetStepName();
            stepStart.timestampMs = m_clock->GetCurrentTimeMs();
            stepStart.stepState = StepState::Running;
            stepStart.progress = instance.GetProgress();
            NotifyObservers(stepStart);

            // Delegate step execution to polymorphic IStepRunner
            auto runner = StepRunnerFactory::CreateRunner(*step);
            auto& stepState = instance.GetStepExecutionState(stepId);
            DefaultBindingResolver bindingResolver;
            ExecutionOutcome outcome = runner->RunStep(*step, stepState, instance.GetContext(), *m_infrastructure, bindingResolver, instance.GetCancellationToken());
            ExecutionResult stepResult = outcome.status;

            executedSteps.push_back(step);

            if (outcome.IsSuccess()) {
                instance.SetStepState(stepId, StepState::Succeeded);

                ExecutionEvent stepDone;
                stepDone.type = ExecutionEventType::StepCompleted;
                stepDone.instanceId = instance.GetInstanceId();
                stepDone.planId = plan->GetPlanId();
                stepDone.stepId = stepId;
                stepDone.stepName = step->GetStepName();
                stepDone.timestampMs = m_clock->GetCurrentTimeMs();
                stepDone.stepState = StepState::Succeeded;
                stepDone.progress = instance.GetProgress();
                NotifyObservers(stepDone);

                finalResult = stepResult;
            } else {
                instance.SetStepState(stepId, StepState::Failed);

                ExecutionEvent stepFail;
                stepFail.type = ExecutionEventType::StepFailed;
                stepFail.instanceId = instance.GetInstanceId();
                stepFail.planId = plan->GetPlanId();
                stepFail.stepId = stepId;
                stepFail.stepName = step->GetStepName();
                stepFail.timestampMs = m_clock->GetCurrentTimeMs();
                stepFail.stepState = StepState::Failed;
                stepFail.errorMessage = stepResult.errorMessage;
                stepFail.progress = instance.GetProgress();
                NotifyObservers(stepFail);

                // Reverse-order rollback pass
                PerformRollback(instance, executedSteps);

                instance.SetState(PlanState::Failed);

                ExecutionEvent planFail;
                planFail.type = ExecutionEventType::PlanFailed;
                planFail.instanceId = instance.GetInstanceId();
                planFail.planId = plan->GetPlanId();
                planFail.timestampMs = m_clock->GetCurrentTimeMs();
                planFail.planState = PlanState::Failed;
                planFail.errorMessage = stepResult.errorMessage;
                planFail.progress = instance.GetProgress();
                NotifyObservers(planFail);

                return stepResult;
            }
        }
    }

    instance.SetState(PlanState::Completed);

    ExecutionEvent finishEvent;
    finishEvent.type = ExecutionEventType::PlanFinished;
    finishEvent.instanceId = instance.GetInstanceId();
    finishEvent.planId = plan->GetPlanId();
    finishEvent.timestampMs = m_clock->GetCurrentTimeMs();
    finishEvent.planState = PlanState::Completed;
    finishEvent.progress = instance.GetProgress();
    NotifyObservers(finishEvent);

    return finalResult;
}

void ExecutionPlanExecutor::PerformRollback(ExecutionPlanInstance& instance, const std::vector<std::shared_ptr<IExecutionStep>>& executedSteps) {
    instance.SetState(PlanState::RollingBack);

    ExecutionEvent rbStart;
    rbStart.type = ExecutionEventType::RollbackStarted;
    rbStart.instanceId = instance.GetInstanceId();
    rbStart.planId = instance.GetPlan() ? instance.GetPlan()->GetPlanId() : "";
    rbStart.timestampMs = m_clock->GetCurrentTimeMs();
    rbStart.planState = PlanState::RollingBack;
    NotifyObservers(rbStart);

    // Reverse order traversal with error isolation
    for (auto it = executedSteps.rbegin(); it != executedSteps.rend(); ++it) {
        auto step = *it;
        if (!step) continue;
        if (step->GetCapabilities().supportsRollback) {
            if (auto* rbStep = step->AsRollbackCapable()) {
                rbStep->Rollback(instance.GetContext(), *m_infrastructure);
                instance.SetStepState(step->GetStepId(), StepState::RolledBack);
            }
        }
    }

    instance.SetState(PlanState::RolledBack);

    ExecutionEvent rbDone;
    rbDone.type = ExecutionEventType::RollbackFinished;
    rbDone.instanceId = instance.GetInstanceId();
    rbDone.planId = instance.GetPlan() ? instance.GetPlan()->GetPlanId() : "";
    rbDone.timestampMs = m_clock->GetCurrentTimeMs();
    rbDone.planState = PlanState::RolledBack;
    NotifyObservers(rbDone);
}

} // namespace Plan
} // namespace NetDiscovery
