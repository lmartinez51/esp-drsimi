/**
 * @file BinaryExpression.h
 * @brief Binary expression node (two children + operator) (v6.0 Phase D).
 *
 * Supports arithmetic, comparison, and logical operators between two child expressions.
 */

#pragma once

#include "expressions/IExpression.h"
#include <memory>

namespace NetDiscovery {
namespace Expressions {

enum class BinaryOperator : uint8_t {
    // Arithmetic
    Add,
    Subtract,
    Multiply,
    Divide,
    // Comparison
    Equal,
    NotEqual,
    Less,
    LessOrEqual,
    Greater,
    GreaterOrEqual,
    // Logical
    LogicalAnd,   ///< Short-circuits: does not evaluate right if left is false
    LogicalOr     ///< Short-circuits: does not evaluate right if left is true
};

class BinaryExpression final : public IExpression {
public:
    BinaryExpression(BinaryOperator             op,
                     std::shared_ptr<IExpression> left,
                     std::shared_ptr<IExpression> right)
        : m_op(op), m_left(std::move(left)), m_right(std::move(right)) {}

    NetDiscovery::Plan::ExecutionValue Evaluate(
        const NetDiscovery::Plan::ExecutionPlanContext& context,
        const IVariableResolver&                       resolver) const override;

    BinaryOperator                      GetOperator() const { return m_op; }
    const std::shared_ptr<IExpression>& GetLeft()     const { return m_left; }
    const std::shared_ptr<IExpression>& GetRight()    const { return m_right; }

private:
    BinaryOperator               m_op;
    std::shared_ptr<IExpression> m_left;
    std::shared_ptr<IExpression> m_right;
};

} // namespace Expressions
} // namespace NetDiscovery
