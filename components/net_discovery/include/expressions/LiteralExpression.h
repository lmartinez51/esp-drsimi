/**
 * @file LiteralExpression.h
 * @brief Leaf expression node holding an inline ExecutionValue (v6.0 Phase D).
 */

#pragma once

#include "expressions/IExpression.h"
#include "plan/ExecutionValue.h"

namespace NetDiscovery {
namespace Expressions {

class LiteralExpression final : public IExpression {
public:
    explicit LiteralExpression(NetDiscovery::Plan::ExecutionValue value)
        : m_value(std::move(value)) {}

    NetDiscovery::Plan::ExecutionValue Evaluate(
        const NetDiscovery::Plan::ExecutionPlanContext& /*context*/,
        const IVariableResolver&                       /*resolver*/) const override
    {
        return m_value;
    }

private:
    NetDiscovery::Plan::ExecutionValue m_value;
};

} // namespace Expressions
} // namespace NetDiscovery
