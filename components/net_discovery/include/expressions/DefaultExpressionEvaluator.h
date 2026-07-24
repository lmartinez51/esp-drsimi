/**
 * @file DefaultExpressionEvaluator.h
 * @brief Default IExpressionEvaluator implementation (v6.0 Phase D).
 *
 * Delegates directly to IExpression::Evaluate(). No additional overhead.
 * The separation exists so future evaluators (instrumented, cached, sandboxed)
 * can replace DefaultExpressionEvaluator without modifying runners.
 */

#pragma once

#include "expressions/IExpressionEvaluator.h"

namespace NetDiscovery {
namespace Expressions {

class DefaultExpressionEvaluator final : public IExpressionEvaluator {
public:
    NetDiscovery::Plan::ExecutionValue Evaluate(
        const IExpression&                             expr,
        const NetDiscovery::Plan::ExecutionPlanContext& context,
        const IVariableResolver&                       resolver) const override
    {
        return expr.Evaluate(context, resolver);
    }
};

} // namespace Expressions
} // namespace NetDiscovery
