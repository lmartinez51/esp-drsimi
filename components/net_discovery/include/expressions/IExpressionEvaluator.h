/**
 * @file IExpressionEvaluator.h
 * @brief Interface for evaluating an IExpression (v6.0 Phase D).
 *
 * IExpressionEvaluator is a platform-level service consumed by IBindingResolver
 * and step runners. Future evaluators may add instrumentation, sandboxing, or
 * cached evaluation without modifying any runner.
 */

#pragma once

#include "expressions/IExpression.h"
#include "expressions/IVariableResolver.h"
#include "plan/ExecutionValue.h"
#include "plan/ExecutionPlanContext.h"

namespace NetDiscovery {
namespace Expressions {

class IExpressionEvaluator {
public:
    virtual ~IExpressionEvaluator() = default;

    virtual NetDiscovery::Plan::ExecutionValue Evaluate(
        const IExpression&                             expr,
        const NetDiscovery::Plan::ExecutionPlanContext& context,
        const IVariableResolver&                       resolver) const = 0;
};

} // namespace Expressions
} // namespace NetDiscovery
