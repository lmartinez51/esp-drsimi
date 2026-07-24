/**
 * @file ActionStep.h
 * @brief ActionStep wrapping a BoundExecutionRequest (v6.0 Phase C / Phase D Promoted).
 */

#pragma once

#include "plan/IExecutionStep.h"
#include "core/BoundExecutionRequest.h"
#include "plan/binding/StepInputBinding.h"
#include "plan/binding/StepOutputDescriptor.h"
#include "plan/steps/StepMetadata.h"
#include <vector>

namespace NetDiscovery {
namespace Plan {

class ActionStep : public IExecutionStep {
public:
    ActionStep(std::string stepId,
               std::string stepName,
               BoundExecutionRequest boundRequest,
               std::vector<StepInputBinding> inputBindings = {},
               std::vector<StepOutputDescriptor> outputDescriptors = {})
        : m_stepId(std::move(stepId))
        , m_stepName(std::move(stepName))
        , m_boundRequest(std::move(boundRequest))
        , m_inputBindings(std::move(inputBindings))
        , m_outputDescriptors(std::move(outputDescriptors))
    {
        m_metadata.stepId = m_stepId;
        m_metadata.stepName = m_stepName;
    }

    std::string GetStepId() const override { return m_stepId; }
    std::string GetStepName() const override { return m_stepName; }
    StepType GetStepType() const override { return StepType::Action; }
    ExecutionCapabilities GetCapabilities() const override { return ExecutionCapabilities::ActionStepDefaults(); }

    const std::vector<StepInputBinding>& GetInputBindings() const override { return m_inputBindings; }
    const std::vector<StepOutputDescriptor>& GetOutputDescriptors() const override { return m_outputDescriptors; }
    const StepMetadata& GetMetadata() const override { return m_metadata; }

    const BoundExecutionRequest& GetBoundRequest() const { return m_boundRequest; }
    BoundExecutionRequest& GetBoundRequest() { return m_boundRequest; }

private:
    std::string m_stepId;
    std::string m_stepName;
    BoundExecutionRequest m_boundRequest;
    std::vector<StepInputBinding> m_inputBindings;
    std::vector<StepOutputDescriptor> m_outputDescriptors;
    StepMetadata m_metadata;
};

} // namespace Plan
} // namespace NetDiscovery
