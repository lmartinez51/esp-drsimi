/**
 * @file IPredicate.h
 * @brief Abstract predicate interface returning a boolean evaluation (v6.0 Phase D).
 *
 * IPredicate is built on top of the IExpression hierarchy. Predicates are the
 * primary mechanism for condition evaluation in BranchStep, ConditionStep, and LoopStep.
 */

#pragma once

#include "expressions/IVariableResolver.h"
#include "plan/ExecutionPlanContext.h"

namespace NetDiscovery {
namespace Expressions {

class IPredicate {
public:
    virtual ~IPredicate() = default;

    /// Evaluates this predicate against the given context and resolver.
    /// Must be pure: no side effects, no mutation.
    virtual bool Evaluate(
        const NetDiscovery::Plan::ExecutionPlanContext& context,
        const IVariableResolver&                       resolver) const = 0;
};

} // namespace Expressions
} // namespace NetDiscovery
