/**
 * @file BlackboardVariableResolver.h
 * @brief IVariableResolver implementation backed by ExecutionPlanContext (v6.0 Phase D).
 */

#pragma once

#include "expressions/IVariableResolver.h"

namespace NetDiscovery {
namespace Expressions {

class BlackboardVariableResolver final : public IVariableResolver {
public:
    std::optional<NetDiscovery::Plan::ExecutionValue> Resolve(
        const VariableRef&                              ref,
        const NetDiscovery::Plan::ExecutionPlanContext& context) const override
    {
        return context.GetRawValue(ref.key);
    }
};

} // namespace Expressions
} // namespace NetDiscovery
