/**
 * @file VariableExpression.h
 * @brief Leaf expression node that resolves a VariableRef at evaluation time (v6.0 Phase D).
 */

#pragma once

#include "expressions/IExpression.h"
#include "expressions/VariableRef.h"

namespace NetDiscovery {
namespace Expressions {

class VariableExpression final : public IExpression {
public:
    explicit VariableExpression(VariableRef ref)
        : m_ref(std::move(ref)) {}

    NetDiscovery::Plan::ExecutionValue Evaluate(
        const NetDiscovery::Plan::ExecutionPlanContext& context,
        const IVariableResolver&                       resolver) const override
    {
        auto opt = resolver.Resolve(m_ref, context);
        if (opt.has_value()) {
            return *opt;
        }
        // Resolution failure → return monostate (null value)
        return NetDiscovery::Plan::ExecutionValue{};
    }

    const VariableRef& GetRef() const { return m_ref; }

private:
    VariableRef m_ref;
};

} // namespace Expressions
} // namespace NetDiscovery
