/**
 * @file UnaryExpression.h
 * @brief Unary expression node (single child + operator) (v6.0 Phase D).
 *
 * Supports:
 *   Negate    — numeric negation of int64_t or double child
 *   LogicalNot — boolean inversion of bool child
 */

#pragma once

#include "expressions/IExpression.h"
#include <memory>

namespace NetDiscovery {
namespace Expressions {

enum class UnaryOperator : uint8_t {
    Negate,     ///< -(numeric)
    LogicalNot  ///< !(bool)
};

class UnaryExpression final : public IExpression {
public:
    UnaryExpression(UnaryOperator op, std::shared_ptr<IExpression> child)
        : m_op(op), m_child(std::move(child)) {}

    NetDiscovery::Plan::ExecutionValue Evaluate(
        const NetDiscovery::Plan::ExecutionPlanContext& context,
        const IVariableResolver&                       resolver) const override;

    UnaryOperator                     GetOperator() const { return m_op; }
    const std::shared_ptr<IExpression>& GetChild()  const { return m_child; }

private:
    UnaryOperator               m_op;
    std::shared_ptr<IExpression> m_child;
};

} // namespace Expressions
} // namespace NetDiscovery
