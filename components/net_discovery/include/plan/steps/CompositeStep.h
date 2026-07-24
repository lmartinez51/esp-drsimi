/**
 * @file CompositeStep.h
 * @brief CompositeStep pure immutable descriptor with CompositePolicy (v6.0 Phase D).
 */

#pragma once

#include "plan/IExecutionStep.h"
#include "plan/steps/StepMetadata.h"
#include <vector>
#include <memory>

namespace NetDiscovery {
namespace Plan {

enum class CompositePolicy {
    StopOnFailure,
    ContinueOnFailure,
    RollbackOnFailure
};

class CompositeStep : public IExecutionStep {
public:
    CompositeStep(std::string id,
                  std::string name,
                  std::vector<std::shared_ptr<IExecutionStep>> childSteps,
                  CompositePolicy policy = CompositePolicy::StopOnFailure)
        : m_id(std::move(id))
        , m_name(std::move(name))
        , m_childSteps(std::move(childSteps))
        , m_policy(policy)
    {
        m_metadata.stepId = m_id;
        m_metadata.stepName = m_name;
    }

    std::string GetStepId() const override { return m_id; }
    std::string GetStepName() const override { return m_name; }
    StepType GetStepType() const override { return StepType::Composite; }
    ExecutionCapabilities GetCapabilities() const override { return ExecutionCapabilities::ControlStepDefaults(); }
    const StepMetadata& GetMetadata() const override { return m_metadata; }

    const std::vector<std::shared_ptr<IExecutionStep>>& GetChildSteps() const { return m_childSteps; }
    CompositePolicy GetPolicy() const { return m_policy; }

private:
    std::string m_id;
    std::string m_name;
    std::vector<std::shared_ptr<IExecutionStep>> m_childSteps;
    CompositePolicy m_policy;
    StepMetadata m_metadata;
};

} // namespace Plan
} // namespace NetDiscovery
