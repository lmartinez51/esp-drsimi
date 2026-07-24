/**
 * @file IExpression.h
 * @brief Abstract base for the polymorphic expression hierarchy (v6.0 Phase D).
 *
 * IExpression is the root of the platform-level expression tree. All expression
 * nodes implement this interface. Trees are immutable after construction.
 *
 * Concrete types: LiteralExpression, VariableExpression, UnaryExpression, BinaryExpression.
 * All are assembled via ExpressionBuilder and evaluated via IExpressionEvaluator or
 * directly through IExpression::Evaluate().
 *
 * -fno-rtti: no dynamic_cast. Type discrimination is not needed at the evaluator
 * boundary because each concrete class self-evaluates.
 * -fno-exceptions: all evaluation is infallible given correct tree construction;
 * missing variables return monostate (ExecutionValue default).
 */

#pragma once

#include "expressions/IVariableResolver.h"
#include "plan/ExecutionValue.h"
#include "plan/ExecutionPlanContext.h"

namespace NetDiscovery {
namespace Expressions {

class IExpression {
public:
    virtual ~IExpression() = default;

    /// Evaluates this expression node against the given context and resolver.
    /// Must be pure: no side effects, no mutation of context or resolver.
    /// Returns ExecutionValue{std::monostate{}} on resolution failure.
    virtual NetDiscovery::Plan::ExecutionValue Evaluate(
        const NetDiscovery::Plan::ExecutionPlanContext& context,
        const IVariableResolver&                       resolver) const = 0;
};

} // namespace Expressions
} // namespace NetDiscovery
