/**
 * @file DefaultBindingResolver.h
 * @brief Concrete IBindingResolver implementation (v6.0 Phase D).
 */

#pragma once

#include "plan/binding/IBindingResolver.h"

namespace NetDiscovery {
namespace Plan {

class DefaultBindingResolver final : public IBindingResolver {
public:
    ResolvedInputs ResolveInputs(
        const std::vector<StepInputBinding>& bindings,
        const ExecutionPlanContext& context,
        const NetDiscovery::Expressions::IExpressionEvaluator& evaluator,
        const NetDiscovery::Expressions::IVariableResolver& resolver) const override
    {
        ResolvedInputs res;
        for (const auto& binding : bindings) {
            if (binding.expression) {
                ExecutionValue val = evaluator.Evaluate(*binding.expression, context, resolver);
                res.values.emplace_back(binding.name, std::move(val));
            }
        }
        return res;
    }

    void PropagateOutput(
        const StepOutputDescriptor& descriptor,
        const ExecutionOutcome& outcome,
        ExecutionPlanContext& context) const override
    {
        if (!descriptor.targetBlackboardKey.empty() && outcome.outputPayload.has_value()) {
            context.SetValue(descriptor.targetBlackboardKey, *outcome.outputPayload);
        }
    }
};

} // namespace Plan
} // namespace NetDiscovery
