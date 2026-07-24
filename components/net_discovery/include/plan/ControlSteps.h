/**
 * @file ControlSteps.h
 * @brief Concrete control workflow steps (ConditionStep, DelayStep, ParallelStep) (v6.0 Phase C / Phase D Promoted).
 */

#pragma once

#include "plan/IExecutionStep.h"
#include "expressions/IPredicate.h"
#include "plan/steps/StepMetadata.h"
#include <chrono>
#include <vector>
#include <memory>

namespace NetDiscovery {
namespace Plan {

enum class ParallelPolicy {
    FailFast,
    WaitAll
};

class ConditionStep : public IExecutionStep {
public:
    ConditionStep(std::string stepId,
                  std::string stepName,
                  std::shared_ptr<NetDiscovery::Expressions::IPredicate> predicate)
        : m_stepId(std::move(stepId))
        , m_stepName(std::move(stepName))
        , m_predicate(std::move(predicate))
    {
        m_metadata.stepId = m_stepId;
        m_metadata.stepName = m_stepName;
    }

    std::string GetStepId() const override { return m_stepId; }
    std::string GetStepName() const override { return m_stepName; }
    StepType GetStepType() const override { return StepType::Condition; }
    ExecutionCapabilities GetCapabilities() const override { return ExecutionCapabilities::ControlStepDefaults(); }
    const StepMetadata& GetMetadata() const override { return m_metadata; }

    const std::shared_ptr<NetDiscovery::Expressions::IPredicate>& GetPredicate() const { return m_predicate; }

private:
    std::string m_stepId;
    std::string m_stepName;
    std::shared_ptr<NetDiscovery::Expressions::IPredicate> m_predicate;
    StepMetadata m_metadata;
};

class DelayStep : public IExecutionStep {
public:
    DelayStep(std::string stepId, std::string stepName, std::chrono::milliseconds delayMs)
        : m_stepId(std::move(stepId))
        , m_stepName(std::move(stepName))
        , m_delayMs(delayMs)
    {
        m_metadata.stepId = m_stepId;
        m_metadata.stepName = m_stepName;
    }

    std::string GetStepId() const override { return m_stepId; }
    std::string GetStepName() const override { return m_stepName; }
    StepType GetStepType() const override { return StepType::Delay; }
    ExecutionCapabilities GetCapabilities() const override {
        auto caps = ExecutionCapabilities::ControlStepDefaults();
        caps.mayBlock = true;
        return caps;
    }
    const StepMetadata& GetMetadata() const override { return m_metadata; }

    std::chrono::milliseconds GetDelayMs() const { return m_delayMs; }

private:
    std::string m_stepId;
    std::string m_stepName;
    std::chrono::milliseconds m_delayMs;
    StepMetadata m_metadata;
};

class ParallelStep : public IExecutionStep {
public:
    ParallelStep(std::string stepId,
                 std::string stepName,
                 std::vector<std::shared_ptr<IExecutionStep>> childSteps,
                 ParallelPolicy policy = ParallelPolicy::FailFast)
        : m_stepId(std::move(stepId))
        , m_stepName(std::move(stepName))
        , m_childSteps(std::move(childSteps))
        , m_policy(policy)
    {
        m_metadata.stepId = m_stepId;
        m_metadata.stepName = m_stepName;
    }

    std::string GetStepId() const override { return m_stepId; }
    std::string GetStepName() const override { return m_stepName; }
    StepType GetStepType() const override { return StepType::Parallel; }
    ExecutionCapabilities GetCapabilities() const override { return ExecutionCapabilities::ControlStepDefaults(); }
    const StepMetadata& GetMetadata() const override { return m_metadata; }

    const std::vector<std::shared_ptr<IExecutionStep>>& GetChildSteps() const { return m_childSteps; }
    ParallelPolicy GetPolicy() const { return m_policy; }

private:
    std::string m_stepId;
    std::string m_stepName;
    std::vector<std::shared_ptr<IExecutionStep>> m_childSteps;
    ParallelPolicy m_policy;
    StepMetadata m_metadata;
};

} // namespace Plan
} // namespace NetDiscovery
