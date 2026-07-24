/**
 * @file StepRunnerRegistry.cpp
 * @brief StepRunnerRegistry implementation with registration of built-in Phase D step runners.
 */

#include "plan/runners/StepRunnerRegistry.h"
#include "plan/ActionStep.h"
#include "plan/ControlSteps.h"
#include "plan/steps/WaitStep.h"
#include "plan/steps/EventWaitStep.h"
#include "plan/steps/BranchStep.h"
#include "plan/steps/SwitchStep.h"
#include "plan/steps/LoopStep.h"
#include "plan/steps/CompositeStep.h"
#include "plan/IRollbackCapable.h"
#include "expressions/DefaultExpressionEvaluator.h"
#include "expressions/BlackboardVariableResolver.h"
#include "esp_log.h"
#include <thread>

static const char* TAG = "StepRunner";

namespace NetDiscovery {
namespace Plan {

// ---------------------------------------------------------------------------
// ActionStepRunner
// ---------------------------------------------------------------------------
class ActionStepRunner : public IStepRunner {
public:
    ExecutionOutcome RunStep(IExecutionStep&          step,
                             StepExecutionState&      state,
                             ExecutionPlanContext&     context,
                             ExecutionInfrastructure& infrastructure,
                             const IBindingResolver&  bindingResolver,
                             CancellationToken        cancelToken) override
    {
        if (step.GetStepType() != StepType::Action) {
            ExecutionResult res;
            res.status = ExecutionStatus::ExecutionFailed;
            res.errorMessage = "Not an ActionStep";
            return ExecutionOutcome::Failure(res);
        }

        auto* actionStep = static_cast<ActionStep*>(&step);
        state.attemptCount++;

        if (cancelToken.IsCancelled()) {
            return ExecutionOutcome::Cancelled();
        }

        NetDiscovery::Expressions::DefaultExpressionEvaluator evaluator;
        NetDiscovery::Expressions::BlackboardVariableResolver resolver;
        bindingResolver.ResolveInputs(actionStep->GetInputBindings(), context, evaluator, resolver);

        ESP_LOGI(TAG, "ActionStepRunner: Dispatching '%s' via ExecutionInfrastructure", step.GetStepName().c_str());
        ExecutionResult res = infrastructure.ExecuteWithPolicy(actionStep->GetBoundRequest());

        // Store result in blackboard for legacy compatibility
        context.SetValue(step.GetStepId() + ".result", res);

        ExecutionOutcome outcome;
        outcome.status = res;
        outcome.transition = ExecutionTransition::Continue;

        if (res.status == ExecutionStatus::Success) {
            for (const auto& outDesc : actionStep->GetOutputDescriptors()) {
                bindingResolver.PropagateOutput(outDesc, outcome, context);
            }
        }

        return outcome;
    }
};

// ---------------------------------------------------------------------------
// ConditionStepRunner
// ---------------------------------------------------------------------------
class ConditionStepRunner : public IStepRunner {
public:
    ExecutionOutcome RunStep(IExecutionStep&          step,
                             StepExecutionState&      state,
                             ExecutionPlanContext&     context,
                             ExecutionInfrastructure& /*infrastructure*/,
                             const IBindingResolver&  /*bindingResolver*/,
                             CancellationToken        cancelToken) override
    {
        if (step.GetStepType() != StepType::Condition) {
            ExecutionResult res;
            res.status = ExecutionStatus::ExecutionFailed;
            res.errorMessage = "Not a ConditionStep";
            return ExecutionOutcome::Failure(res);
        }

        if (cancelToken.IsCancelled()) {
            return ExecutionOutcome::Cancelled();
        }

        auto* condStep = static_cast<ConditionStep*>(&step);
        state.attemptCount++;

        NetDiscovery::Expressions::BlackboardVariableResolver resolver;
        bool result = false;
        if (condStep->GetPredicate()) {
            result = condStep->GetPredicate()->Evaluate(context, resolver);
        }

        context.SetValue(step.GetStepId() + ".eval", result);

        ExecutionOutcome outcome;
        outcome.status.status = result ? ExecutionStatus::Success : ExecutionStatus::ExecutionFailed;
        outcome.transition = result ? ExecutionTransition::BranchTrue : ExecutionTransition::BranchFalse;
        outcome.outputPayload = ExecutionValue{result};
        return outcome;
    }
};

// ---------------------------------------------------------------------------
// DelayStepRunner
// ---------------------------------------------------------------------------
class DelayStepRunner : public IStepRunner {
public:
    ExecutionOutcome RunStep(IExecutionStep&          step,
                             StepExecutionState&      state,
                             ExecutionPlanContext&     /*context*/,
                             ExecutionInfrastructure& /*infrastructure*/,
                             const IBindingResolver&  /*bindingResolver*/,
                             CancellationToken        cancelToken) override
    {
        if (step.GetStepType() != StepType::Delay) {
            ExecutionResult res;
            res.status = ExecutionStatus::ExecutionFailed;
            res.errorMessage = "Not a DelayStep";
            return ExecutionOutcome::Failure(res);
        }

        auto* delayStep = static_cast<DelayStep*>(&step);
        state.attemptCount++;

        auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < delayStep->GetDelayMs()) {
            if (cancelToken.IsCancelled()) {
                return ExecutionOutcome::Cancelled();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        return ExecutionOutcome::Success();
    }
};

// ---------------------------------------------------------------------------
// WaitStepRunner
// ---------------------------------------------------------------------------
class WaitStepRunner : public IStepRunner {
public:
    ExecutionOutcome RunStep(IExecutionStep&          step,
                             StepExecutionState&      state,
                             ExecutionPlanContext&     context,
                             ExecutionInfrastructure& /*infrastructure*/,
                             const IBindingResolver&  /*bindingResolver*/,
                             CancellationToken        cancelToken) override
    {
        if (step.GetStepType() != StepType::Wait) {
            ExecutionResult res;
            res.status = ExecutionStatus::ExecutionFailed;
            res.errorMessage = "Not a WaitStep";
            return ExecutionOutcome::Failure(res);
        }

        auto* waitStep = static_cast<WaitStep*>(&step);
        state.attemptCount++;

        NetDiscovery::Expressions::BlackboardVariableResolver resolver;
        auto start = std::chrono::steady_clock::now();

        while (true) {
            if (cancelToken.IsCancelled()) {
                return ExecutionOutcome::Cancelled();
            }
            auto opt = resolver.Resolve(waitStep->GetVariableRef(), context);
            if (opt.has_value()) {
                ExecutionOutcome outcome = ExecutionOutcome::Success();
                outcome.outputPayload = opt;
                return outcome;
            }
            if (waitStep->GetTimeoutMs().count() > 0 &&
                (std::chrono::steady_clock::now() - start) >= waitStep->GetTimeoutMs())
            {
                ExecutionResult res;
                res.status = ExecutionStatus::ExecutionFailed;
                res.errorMessage = "WaitStep timed out";
                return ExecutionOutcome::Failure(res);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
};

// ---------------------------------------------------------------------------
// EventWaitStepRunner
// ---------------------------------------------------------------------------
class EventWaitStepRunner : public IStepRunner {
public:
    ExecutionOutcome RunStep(IExecutionStep&          step,
                             StepExecutionState&      state,
                             ExecutionPlanContext&     /*context*/,
                             ExecutionInfrastructure& /*infrastructure*/,
                             const IBindingResolver&  /*bindingResolver*/,
                             CancellationToken        cancelToken) override
    {
        if (step.GetStepType() != StepType::EventWait) {
            ExecutionResult res;
            res.status = ExecutionStatus::ExecutionFailed;
            res.errorMessage = "Not an EventWaitStep";
            return ExecutionOutcome::Failure(res);
        }

        auto* eventStep = static_cast<EventWaitStep*>(&step);
        state.attemptCount++;

        WaitResult wr = eventStep->GetSignal().Wait(eventStep->GetTimeoutMs(), cancelToken);
        switch (wr) {
            case WaitResult::Signalled:
                return ExecutionOutcome::Success();
            case WaitResult::Cancelled:
                return ExecutionOutcome::Cancelled();
            case WaitResult::TimedOut:
            default:
            {
                ExecutionResult res;
                res.status = ExecutionStatus::ExecutionFailed;
                res.errorMessage = "EventWaitStep timed out";
                return ExecutionOutcome::Failure(res);
            }
        }
    }
};

// ---------------------------------------------------------------------------
// BranchStepRunner
// ---------------------------------------------------------------------------
class BranchStepRunner : public IStepRunner {
public:
    ExecutionOutcome RunStep(IExecutionStep&          step,
                             StepExecutionState&      state,
                             ExecutionPlanContext&     context,
                             ExecutionInfrastructure& /*infrastructure*/,
                             const IBindingResolver&  /*bindingResolver*/,
                             CancellationToken        cancelToken) override
    {
        if (step.GetStepType() != StepType::Branch) {
            ExecutionResult res;
            res.status = ExecutionStatus::ExecutionFailed;
            res.errorMessage = "Not a BranchStep";
            return ExecutionOutcome::Failure(res);
        }

        if (cancelToken.IsCancelled()) {
            return ExecutionOutcome::Cancelled();
        }

        auto* branchStep = static_cast<BranchStep*>(&step);
        state.attemptCount++;

        NetDiscovery::Expressions::BlackboardVariableResolver resolver;
        bool predRes = false;
        if (branchStep->GetPredicate()) {
            predRes = branchStep->GetPredicate()->Evaluate(context, resolver);
        }

        // Store observable data in blackboard (data flow only, NOT routing)
        context.SetValue(step.GetStepId() + ".branch.result", predRes);

        ExecutionTransition trans = predRes ? ExecutionTransition::BranchTrue : ExecutionTransition::BranchFalse;
        ExecutionResult succRes;
        succRes.status = ExecutionStatus::Success;
        ExecutionOutcome outcome = ExecutionOutcome::WithTransition(succRes, trans);
        outcome.outputPayload = ExecutionValue{predRes};
        return outcome;
    }
};

// ---------------------------------------------------------------------------
// SwitchStepRunner
// ---------------------------------------------------------------------------
class SwitchStepRunner : public IStepRunner {
public:
    ExecutionOutcome RunStep(IExecutionStep&          step,
                             StepExecutionState&      state,
                             ExecutionPlanContext&     context,
                             ExecutionInfrastructure& /*infrastructure*/,
                             const IBindingResolver&  /*bindingResolver*/,
                             CancellationToken        cancelToken) override
    {
        if (step.GetStepType() != StepType::Switch) {
            ExecutionResult res;
            res.status = ExecutionStatus::ExecutionFailed;
            res.errorMessage = "Not a SwitchStep";
            return ExecutionOutcome::Failure(res);
        }

        if (cancelToken.IsCancelled()) {
            return ExecutionOutcome::Cancelled();
        }

        auto* switchStep = static_cast<SwitchStep*>(&step);
        state.attemptCount++;

        NetDiscovery::Expressions::DefaultExpressionEvaluator evaluator;
        NetDiscovery::Expressions::BlackboardVariableResolver resolver;

        ExecutionValue val;
        if (switchStep->GetExpression()) {
            val = evaluator.Evaluate(*switchStep->GetExpression(), context, resolver);
        }

        context.SetValue(step.GetStepId() + ".switch.value", val);

        std::string targetNode = switchStep->GetDefaultNodeId();
        for (const auto& c : switchStep->GetCases()) {
            if (c.matchValue == val) {
                targetNode = c.targetNodeId;
                break;
            }
        }

        ExecutionOutcome outcome = ExecutionOutcome::Success();
        outcome.outputPayload = val;

        // Signal switch target node if non-empty
        if (!targetNode.empty()) {
            context.SetValue(step.GetStepId() + ".switch.target", targetNode);
        }

        return outcome;
    }
};

// ---------------------------------------------------------------------------
// LoopStepRunner
// ---------------------------------------------------------------------------
class LoopStepRunner : public IStepRunner {
public:
    ExecutionOutcome RunStep(IExecutionStep&          step,
                             StepExecutionState&      state,
                             ExecutionPlanContext&     context,
                             ExecutionInfrastructure& /*infrastructure*/,
                             const IBindingResolver&  /*bindingResolver*/,
                             CancellationToken        cancelToken) override
    {
        ExecutionResult failRes;
        failRes.status = ExecutionStatus::ExecutionFailed;

        if (step.GetStepType() != StepType::Loop) {
            failRes.errorMessage = "Not a LoopStep";
            return ExecutionOutcome::Failure(failRes);
        }

        if (cancelToken.IsCancelled()) {
            return ExecutionOutcome::Cancelled();
        }

        auto* loopStep = static_cast<LoopStep*>(&step);

        ExecutionResult succRes;
        succRes.status = ExecutionStatus::Success;

        if (state.attemptCount >= loopStep->GetMaxIterations()) {
            return ExecutionOutcome::WithTransition(succRes, ExecutionTransition::LoopExit);
        }

        NetDiscovery::Expressions::BlackboardVariableResolver resolver;
        bool predRes = false;
        if (loopStep->GetPredicate()) {
            predRes = loopStep->GetPredicate()->Evaluate(context, resolver);
        }

        if (!predRes) {
            return ExecutionOutcome::WithTransition(succRes, ExecutionTransition::LoopExit);
        }

        state.attemptCount++;
        return ExecutionOutcome::WithTransition(succRes, ExecutionTransition::LoopContinue);
    }
};

// ---------------------------------------------------------------------------
// ParallelStepRunner
// ---------------------------------------------------------------------------
class ParallelStepRunner : public IStepRunner {
public:
    ExecutionOutcome RunStep(IExecutionStep&          step,
                             StepExecutionState&      state,
                             ExecutionPlanContext&     context,
                             ExecutionInfrastructure& infrastructure,
                             const IBindingResolver&  bindingResolver,
                             CancellationToken        cancelToken) override
    {
        if (step.GetStepType() != StepType::Parallel) {
            ExecutionResult failRes;
            failRes.status = ExecutionStatus::ExecutionFailed;
            failRes.errorMessage = "Not a ParallelStep";
            return ExecutionOutcome::Failure(failRes);
        }

        auto* parallelStep = static_cast<ParallelStep*>(&step);
        state.attemptCount++;

        const auto& registry = StepRunnerRegistry::Instance();

        for (const auto& child : parallelStep->GetChildSteps()) {
            if (cancelToken.IsCancelled()) {
                return ExecutionOutcome::Cancelled();
            }
            auto runner = registry.CreateRunner(*child);
            if (runner) {
                StepExecutionState childState;
                childState.stepId = child->GetStepId();
                auto childOutcome = runner->RunStep(*child, childState, context, infrastructure, bindingResolver, cancelToken);
                if (!childOutcome.IsSuccess()) {
                    if (parallelStep->GetPolicy() == ParallelPolicy::FailFast) {
                        return childOutcome;
                    }
                }
            }
        }

        return ExecutionOutcome::Success();
    }
};

// ---------------------------------------------------------------------------
// CompositeStepRunner
// ---------------------------------------------------------------------------
class CompositeStepRunner : public IStepRunner {
public:
    ExecutionOutcome RunStep(IExecutionStep&          step,
                             StepExecutionState&      state,
                             ExecutionPlanContext&     context,
                             ExecutionInfrastructure& infrastructure,
                             const IBindingResolver&  bindingResolver,
                             CancellationToken        cancelToken) override
    {
        if (step.GetStepType() != StepType::Composite) {
            ExecutionResult failRes;
            failRes.status = ExecutionStatus::ExecutionFailed;
            failRes.errorMessage = "Not a CompositeStep";
            return ExecutionOutcome::Failure(failRes);
        }

        auto* compStep = static_cast<CompositeStep*>(&step);
        state.attemptCount++;

        const auto& registry = StepRunnerRegistry::Instance();
        std::vector<std::pair<std::shared_ptr<IExecutionStep>, ExecutionOutcome>> executed;
        std::optional<ExecutionOutcome> firstFail;

        for (const auto& child : compStep->GetChildSteps()) {
            if (cancelToken.IsCancelled()) {
                return ExecutionOutcome::Cancelled();
            }
            auto runner = registry.CreateRunner(*child);
            if (runner) {
                StepExecutionState childState;
                childState.stepId = child->GetStepId();
                auto childOutcome = runner->RunStep(*child, childState, context, infrastructure, bindingResolver, cancelToken);
                if (childOutcome.IsSuccess()) {
                    executed.push_back({child, childOutcome});
                } else {
                    if (!firstFail.has_value()) firstFail = childOutcome;

                    switch (compStep->GetPolicy()) {
                        case CompositePolicy::StopOnFailure:
                            return *firstFail;

                        case CompositePolicy::ContinueOnFailure:
                            break;

                        case CompositePolicy::RollbackOnFailure:
                            for (auto it = executed.rbegin(); it != executed.rend(); ++it) {
                                if (auto* rb = it->first->AsRollbackCapable()) {
                                    rb->Rollback(context, infrastructure);
                                }
                            }
                            return *firstFail;
                    }
                }
            }
        }

        return firstFail.value_or(ExecutionOutcome::Success());
    }
};

// ---------------------------------------------------------------------------
// DefaultStepRunner
// ---------------------------------------------------------------------------
class DefaultStepRunner : public IStepRunner {
public:
    ExecutionOutcome RunStep(IExecutionStep&          /*step*/,
                             StepExecutionState&      state,
                             ExecutionPlanContext&     /*context*/,
                             ExecutionInfrastructure& /*infrastructure*/,
                             const IBindingResolver&  /*bindingResolver*/,
                             CancellationToken        /*cancelToken*/) override
    {
        state.attemptCount++;
        return ExecutionOutcome::Success();
    }
};

// ---------------------------------------------------------------------------
// StepRunnerRegistry implementation
// ---------------------------------------------------------------------------
StepRunnerRegistry::StepRunnerRegistry() {
    Register(StepType::Action, [](IExecutionStep&) { return std::make_shared<ActionStepRunner>(); });
    Register(StepType::Condition, [](IExecutionStep&) { return std::make_shared<ConditionStepRunner>(); });
    Register(StepType::Delay, [](IExecutionStep&) { return std::make_shared<DelayStepRunner>(); });
    Register(StepType::Wait, [](IExecutionStep&) { return std::make_shared<WaitStepRunner>(); });
    Register(StepType::EventWait, [](IExecutionStep&) { return std::make_shared<EventWaitStepRunner>(); });
    Register(StepType::Branch, [](IExecutionStep&) { return std::make_shared<BranchStepRunner>(); });
    Register(StepType::Switch, [](IExecutionStep&) { return std::make_shared<SwitchStepRunner>(); });
    Register(StepType::Loop, [](IExecutionStep&) { return std::make_shared<LoopStepRunner>(); });
    Register(StepType::Parallel, [](IExecutionStep&) { return std::make_shared<ParallelStepRunner>(); });
    Register(StepType::Composite, [](IExecutionStep&) { return std::make_shared<CompositeStepRunner>(); });
}

void StepRunnerRegistry::Register(StepType type, RunnerFactoryFn factory) {
    m_factories[type] = std::move(factory);
}

std::shared_ptr<IStepRunner> StepRunnerRegistry::CreateRunner(IExecutionStep& step) const {
    auto it = m_factories.find(step.GetStepType());
    if (it != m_factories.end()) {
        return it->second(step);
    }
    return std::make_shared<DefaultStepRunner>();
}

StepRunnerRegistry& StepRunnerRegistry::Instance() {
    static StepRunnerRegistry s_instance;
    return s_instance;
}

} // namespace Plan
} // namespace NetDiscovery
