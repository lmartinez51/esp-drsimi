/**
 * @file LogicalPredicates.h
 * @brief Concrete logical predicate combinators: And, Or, Not, Value, Compare (v6.0 Phase D).
 */

#pragma once

#include "expressions/IPredicate.h"
#include "expressions/IExpression.h"
#include "expressions/BinaryExpression.h"
#include "plan/ExecutionValue.h"
#include <memory>

namespace NetDiscovery {
namespace Expressions {

// ---------------------------------------------------------------------------
// ValuePredicate — evaluates an IExpression and interprets it as truthy/falsy
// ---------------------------------------------------------------------------
class ValuePredicate final : public IPredicate {
public:
    explicit ValuePredicate(std::shared_ptr<IExpression> expr)
        : m_expr(std::move(expr)) {}

    bool Evaluate(const NetDiscovery::Plan::ExecutionPlanContext& context,
                  const IVariableResolver& resolver) const override;

private:
    std::shared_ptr<IExpression> m_expr;
};

// ---------------------------------------------------------------------------
// ComparePredicate — compares two IExpression values with a BinaryOperator
// ---------------------------------------------------------------------------
class ComparePredicate final : public IPredicate {
public:
    ComparePredicate(std::shared_ptr<IExpression> left,
                     BinaryOperator               op,
                     std::shared_ptr<IExpression> right)
        : m_left(std::move(left)), m_op(op), m_right(std::move(right)) {}

    bool Evaluate(const NetDiscovery::Plan::ExecutionPlanContext& context,
                  const IVariableResolver& resolver) const override;

private:
    std::shared_ptr<IExpression> m_left;
    BinaryOperator               m_op;
    std::shared_ptr<IExpression> m_right;
};

// ---------------------------------------------------------------------------
// LogicalAndPredicate — short-circuit AND of two IPredicate
// ---------------------------------------------------------------------------
class LogicalAndPredicate final : public IPredicate {
public:
    LogicalAndPredicate(std::shared_ptr<IPredicate> left,
                        std::shared_ptr<IPredicate> right)
        : m_left(std::move(left)), m_right(std::move(right)) {}

    bool Evaluate(const NetDiscovery::Plan::ExecutionPlanContext& context,
                  const IVariableResolver& resolver) const override
    {
        // Short-circuit: do not evaluate right if left is false
        return m_left->Evaluate(context, resolver) &&
               m_right->Evaluate(context, resolver);
    }

private:
    std::shared_ptr<IPredicate> m_left;
    std::shared_ptr<IPredicate> m_right;
};

// ---------------------------------------------------------------------------
// LogicalOrPredicate — short-circuit OR of two IPredicate
// ---------------------------------------------------------------------------
class LogicalOrPredicate final : public IPredicate {
public:
    LogicalOrPredicate(std::shared_ptr<IPredicate> left,
                       std::shared_ptr<IPredicate> right)
        : m_left(std::move(left)), m_right(std::move(right)) {}

    bool Evaluate(const NetDiscovery::Plan::ExecutionPlanContext& context,
                  const IVariableResolver& resolver) const override
    {
        // Short-circuit: do not evaluate right if left is true
        return m_left->Evaluate(context, resolver) ||
               m_right->Evaluate(context, resolver);
    }

private:
    std::shared_ptr<IPredicate> m_left;
    std::shared_ptr<IPredicate> m_right;
};

// ---------------------------------------------------------------------------
// LogicalNotPredicate — inversion of one IPredicate
// ---------------------------------------------------------------------------
class LogicalNotPredicate final : public IPredicate {
public:
    explicit LogicalNotPredicate(std::shared_ptr<IPredicate> inner)
        : m_inner(std::move(inner)) {}

    bool Evaluate(const NetDiscovery::Plan::ExecutionPlanContext& context,
                  const IVariableResolver& resolver) const override
    {
        return !m_inner->Evaluate(context, resolver);
    }

private:
    std::shared_ptr<IPredicate> m_inner;
};

} // namespace Expressions
} // namespace NetDiscovery
