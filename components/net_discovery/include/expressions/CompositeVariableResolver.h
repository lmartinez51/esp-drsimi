/**
 * @file CompositeVariableResolver.h
 * @brief Chains multiple IVariableResolver instances (first-match wins) (v6.0 Phase D).
 *
 * Typical use: BlackboardVariableResolver → LiteralVariableResolver (fallback to defaults).
 */

#pragma once

#include "expressions/IVariableResolver.h"
#include <vector>
#include <memory>

namespace NetDiscovery {
namespace Expressions {

class CompositeVariableResolver final : public IVariableResolver {
public:
    void AddResolver(std::shared_ptr<IVariableResolver> resolver) {
        if (resolver) {
            m_resolvers.push_back(std::move(resolver));
        }
    }

    std::optional<NetDiscovery::Plan::ExecutionValue> Resolve(
        const VariableRef&                              ref,
        const NetDiscovery::Plan::ExecutionPlanContext& context) const override
    {
        for (const auto& r : m_resolvers) {
            auto result = r->Resolve(ref, context);
            if (result.has_value()) {
                return result;
            }
        }
        return std::nullopt;
    }

private:
    std::vector<std::shared_ptr<IVariableResolver>> m_resolvers;
};

} // namespace Expressions
} // namespace NetDiscovery
