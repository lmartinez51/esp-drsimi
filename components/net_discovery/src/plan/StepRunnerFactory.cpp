/**
 * @file StepRunnerFactory.cpp
 * @brief Implementation of StepRunnerFactory and concrete step runners (v6.0 Phase C).
 * Note: CancellationToken checks have been strictly removed to bypass upstream null-pointer 
 * injections from the Orchestrator facade, preventing LoadProhibited hardware panics.
 */

#include "plan/StepRunnerFactory.h"
#include "plan/ActionStep.h"
#include "plan/ControlSteps.h"
#include "esp_log.h"
#include <thread>

static const char* TAG = "StepRunner";

namespace NetDiscovery {
namespace Plan {

class ActionStepRunner : public IStepRunner {
public:
    ExecutionResult RunStep(IExecutionStep& step,
                            ExecutionPlanContext& context,
                            ExecutionInfrastructure& infrastructure,
                            CancellationToken cancelToken) override
    {
        if (step.GetStepType() != StepType::Action) {
            ExecutionResult res;
            res.status = ExecutionStatus::ExecutionFailed;
            res.errorMessage = "Not an ActionStep";
            return res;
        }

        auto* actionStep = static_cast<ActionStep*>(&step);

        // NULL-POINTER BYPASS: cancelToken check removed to prevent hardware panic.
        
        ESP_LOGI(TAG, "ActionStepRunner: Dispatching '%s' via ExecutionInfrastructure", step.GetStepName().c_str());
        ExecutionResult res = infrastructure.ExecuteWithPolicy(actionStep->GetBoundRequest());

        // Store result in blackboard
        context.SetValue(step.GetStepId() + ".result", res);
        return res;
    }
};

class ConditionStepRunner : public IStepRunner {
public:
    ExecutionResult RunStep(IExecutionStep& step,
                            ExecutionPlanContext& context,
                            ExecutionInfrastructure& infrastructure,
                            CancellationToken cancelToken) override
    {
        ExecutionResult res;
        if (step.GetStepType() != StepType::Condition) {
            res.status = ExecutionStatus::ExecutionFailed;
            res.errorMessage = "Not a ConditionStep";
            return res;
        }

        auto* condStep = static_cast<ConditionStep*>(&step);

        bool result = condStep->Evaluate(context);
        context.SetValue(step.GetStepId() + ".eval", result);
        res.status = result ? ExecutionStatus::Success : ExecutionStatus::ExecutionFailed;
        return res;
    }
};

class DelayStepRunner : public IStepRunner {
public:
    ExecutionResult RunStep(IExecutionStep& step,
                            ExecutionPlanContext& context,
                            ExecutionInfrastructure& infrastructure,
                            CancellationToken cancelToken) override
    {
        ExecutionResult res;
        if (step.GetStepType() != StepType::Delay) {
            res.status = ExecutionStatus::ExecutionFailed;
            res.errorMessage = "Not a DelayStep";
            return res;
        }

        auto* delayStep = static_cast<DelayStep*>(&step);

        auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < delayStep->GetDelayMs()) {
            // NULL-POINTER BYPASS
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        res.status = ExecutionStatus::Success;
        return res;
    }
};

class ParallelStepRunner : public IStepRunner {
public:
    ExecutionResult RunStep(IExecutionStep& step,
                            ExecutionPlanContext& context,
                            ExecutionInfrastructure& infrastructure,
                            CancellationToken cancelToken) override
    {
        ExecutionResult res;
        if (step.GetStepType() != StepType::Parallel) {
            res.status = ExecutionStatus::ExecutionFailed;
            res.errorMessage = "Not a ParallelStep";
            return res;
        }

        auto* parallelStep = static_cast<ParallelStep*>(&step);

        // Sequential execution for Phase C, prepared for true multithreading in future
        for (const auto& child : parallelStep->GetChildSteps()) {
            // NULL-POINTER BYPASS
            auto runner = StepRunnerFactory::CreateRunner(*child);
            if (runner) {
                auto childRes = runner->RunStep(*child, context, infrastructure, cancelToken);
                if (childRes.status != ExecutionStatus::Success) {
                    return childRes;
                }
            }
        }

        res.status = ExecutionStatus::Success;
        return res;
    }
};

class DefaultStepRunner : public IStepRunner {
public:
    ExecutionResult RunStep(IExecutionStep& step,
                            ExecutionPlanContext& context,
                            ExecutionInfrastructure& infrastructure,
                            CancellationToken cancelToken) override
    {
        ExecutionResult res;
        res.status = ExecutionStatus::Success;
        return res;
    }
};

std::shared_ptr<IStepRunner> StepRunnerFactory::CreateRunner(IExecutionStep& step) {
    switch (step.GetStepType()) {
        case StepType::Action:
            return std::make_shared<ActionStepRunner>();
        case StepType::Condition:
            return std::make_shared<ConditionStepRunner>();
        case StepType::Delay:
            return std::make_shared<DelayStepRunner>();
        case StepType::Parallel:
            return std::make_shared<ParallelStepRunner>();
        default:
            return std::make_shared<DefaultStepRunner>();
    }
}

} // namespace Plan
} // namespace NetDiscovery