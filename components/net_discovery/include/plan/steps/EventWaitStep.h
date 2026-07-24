/**
 * @file EventWaitStep.h
 * @brief EventWaitStep pure immutable descriptor (v6.0 Phase D).
 */

#pragma once

#include "plan/IExecutionStep.h"
#include "plan/events/IEventSignal.h"
#include "plan/steps/StepMetadata.h"
#include <chrono>

namespace NetDiscovery {
namespace Plan {

class EventWaitStep : public IExecutionStep {
public:
    EventWaitStep(std::string id, std::string name, IEventSignal& signal, std::chrono::milliseconds timeoutMs)
        : m_id(std::move(id))
        , m_name(std::move(name))
        , m_signal(signal)
        , m_timeoutMs(timeoutMs)
    {
        m_metadata.stepId = m_id;
        m_metadata.stepName = m_name;
    }

    std::string GetStepId() const override { return m_id; }
    std::string GetStepName() const override { return m_name; }
    StepType GetStepType() const override { return StepType::EventWait; }
    ExecutionCapabilities GetCapabilities() const override { return ExecutionCapabilities::ControlStepDefaults(); }
    const StepMetadata& GetMetadata() const override { return m_metadata; }

    IEventSignal& GetSignal() const { return m_signal; }
    std::chrono::milliseconds GetTimeoutMs() const { return m_timeoutMs; }

private:
    std::string m_id;
    std::string m_name;
    IEventSignal& m_signal;
    std::chrono::milliseconds m_timeoutMs;
    StepMetadata m_metadata;
};

} // namespace Plan
} // namespace NetDiscovery
