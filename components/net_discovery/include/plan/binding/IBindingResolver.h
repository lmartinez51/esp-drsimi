/**
 * @file IBindingResolver.h
 * @brief Interface for input resolution and output propagation service (v6.0 Phase D).
 */

#pragma once

#include "plan/binding/StepInputBinding.h"
#include "plan/binding/StepOutputDescriptor.h"
#include "plan/ExecutionOutcome.h"
#include "plan/ExecutionPlanContext.h"
#include "expressions/IExpressionEvaluator.h"
#include "expressions/IVariableResolver.h"
#include <vector>
#include <utility>
#include <optional>

namespace NetDiscovery {
namespace Plan {

struct ResolvedInputs {
    std::vector<std::pair<std::string, ExecutionValue>> values;

    std::optional<ExecutionValue> Get(const std::string& name) const {
        for (const auto& kv : values) {
            if (kv.first == name) return kv.second;
        }
        return std::nullopt;
    }
};

class IBindingResolver {
public:
    virtual ~IBindingResolver() = default;

    virtual ResolvedInputs ResolveInputs(
        const std::vector<StepInputBinding>& bindings,
        const ExecutionPlanContext& context,
        const NetDiscovery::Expressions::IExpressionEvaluator& evaluator,
        const NetDiscovery::Expressions::IVariableResolver& resolver) const = 0;

    virtual void PropagateOutput(
        const StepOutputDescriptor& descriptor,
        const ExecutionOutcome& outcome,
        ExecutionPlanContext& context) const = 0;
};

} // namespace Plan
} // namespace NetDiscovery
