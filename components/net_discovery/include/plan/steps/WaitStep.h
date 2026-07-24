/**
 * @file WaitStep.h
 * @brief WaitStep pure immutable descriptor (v6.0 Phase D).
 */

#pragma once

#include "plan/IExecutionStep.h"
#include "expressions/VariableRef.h"
#include "plan/steps/StepMetadata.h"
#include <chrono>

namespace NetDiscovery {
namespace Plan {

class WaitStep : public IExecutionStep {
public:
    WaitStep(std::string id, std::string name, NetDiscovery::Expressions::VariableRef ref, std::chrono::milliseconds timeoutMs)
        : m_id(std::move(id))
        , m_name(std::move(name))
        , m_ref(std::move(ref))
        , m_timeoutMs(timeoutMs)
    {
        m_metadata.stepId = m_id;
        m_metadata.stepName = m_name;
    }

    std::string GetStepId() const override { return m_id; }
    std::string GetStepName() const override { return m_name; }
    StepType GetStepType() const override { return StepType::Wait; }
    ExecutionCapabilities GetCapabilities() const override { return ExecutionCapabilities::ControlStepDefaults(); }
    const StepMetadata& GetMetadata() const override { return m_metadata; }

    const NetDiscovery::Expressions::VariableRef& GetVariableRef() const { return m_ref; }
    std::chrono::milliseconds GetTimeoutMs() const { return m_timeoutMs; }

private:
    std::string m_id;
    std::string m_name;
    NetDiscovery::Expressions::VariableRef m_ref;
    std::chrono::milliseconds m_timeoutMs;
    StepMetadata m_metadata;
};

} // namespace Plan
} // namespace NetDiscovery
