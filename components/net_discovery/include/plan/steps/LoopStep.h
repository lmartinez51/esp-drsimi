/**
 * @file LoopStep.h
 * @brief LoopStep pure immutable descriptor (v6.0 Phase D).
 */

#pragma once

#include "plan/IExecutionStep.h"
#include "expressions/IPredicate.h"
#include "plan/steps/StepMetadata.h"
#include <memory>
#include <cstdint>

namespace NetDiscovery {
namespace Plan {

class LoopStep : public IExecutionStep {
public:
    LoopStep(std::string id,
             std::string name,
             std::shared_ptr<NetDiscovery::Expressions::IPredicate> predicate,
             uint8_t maxIterations = 100)
        : m_id(std::move(id))
        , m_name(std::move(name))
        , m_predicate(std::move(predicate))
        , m_maxIterations(maxIterations)
    {
        m_metadata.stepId = m_id;
        m_metadata.stepName = m_name;
    }

    std::string GetStepId() const override { return m_id; }
    std::string GetStepName() const override { return m_name; }
    StepType GetStepType() const override { return StepType::Loop; }
    ExecutionCapabilities GetCapabilities() const override { return ExecutionCapabilities::ControlStepDefaults(); }
    const StepMetadata& GetMetadata() const override { return m_metadata; }

    const std::shared_ptr<NetDiscovery::Expressions::IPredicate>& GetPredicate() const { return m_predicate; }
    uint8_t GetMaxIterations() const { return m_maxIterations; }

private:
    std::string m_id;
    std::string m_name;
    std::shared_ptr<NetDiscovery::Expressions::IPredicate> m_predicate;
    uint8_t m_maxIterations;
    StepMetadata m_metadata;
};

} // namespace Plan
} // namespace NetDiscovery
