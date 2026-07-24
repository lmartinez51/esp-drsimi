/**
 * @file SwitchStep.h
 * @brief SwitchStep pure immutable descriptor (v6.0 Phase D).
 */

#pragma once

#include "plan/IExecutionStep.h"
#include "expressions/IExpression.h"
#include "plan/steps/StepMetadata.h"
#include <memory>
#include <vector>

namespace NetDiscovery {
namespace Plan {

struct SwitchCase {
    ExecutionValue matchValue;
    std::string targetNodeId;
};

class SwitchStep : public IExecutionStep {
public:
    SwitchStep(std::string id,
               std::string name,
               std::shared_ptr<NetDiscovery::Expressions::IExpression> expression,
               std::vector<SwitchCase> cases,
               std::string defaultNodeId)
        : m_id(std::move(id))
        , m_name(std::move(name))
        , m_expression(std::move(expression))
        , m_cases(std::move(cases))
        , m_defaultNodeId(std::move(defaultNodeId))
    {
        m_metadata.stepId = m_id;
        m_metadata.stepName = m_name;
    }

    std::string GetStepId() const override { return m_id; }
    std::string GetStepName() const override { return m_name; }
    StepType GetStepType() const override { return StepType::Switch; }
    ExecutionCapabilities GetCapabilities() const override { return ExecutionCapabilities::ControlStepDefaults(); }
    const StepMetadata& GetMetadata() const override { return m_metadata; }

    const std::shared_ptr<NetDiscovery::Expressions::IExpression>& GetExpression() const { return m_expression; }
    const std::vector<SwitchCase>& GetCases() const { return m_cases; }
    const std::string& GetDefaultNodeId() const { return m_defaultNodeId; }

private:
    std::string m_id;
    std::string m_name;
    std::shared_ptr<NetDiscovery::Expressions::IExpression> m_expression;
    std::vector<SwitchCase> m_cases;
    std::string m_defaultNodeId;
    StepMetadata m_metadata;
};

} // namespace Plan
} // namespace NetDiscovery
