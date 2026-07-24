/**
 * @file LiteralVariableResolver.h
 * @brief IVariableResolver that returns an inline literal for a given key (v6.0 Phase D).
 *
 * Used when a binding source is a constant that does not need to be looked up
 * in the blackboard. Typically composed with CompositeVariableResolver.
 */

#pragma once

#include "expressions/IVariableResolver.h"
#include <unordered_map>
#include <string>

namespace NetDiscovery {
namespace Expressions {

class LiteralVariableResolver final : public IVariableResolver {
public:
    void Set(const std::string& key, NetDiscovery::Plan::ExecutionValue value) {
        m_literals[key] = std::move(value);
    }

    std::optional<NetDiscovery::Plan::ExecutionValue> Resolve(
        const VariableRef&                              ref,
        const NetDiscovery::Plan::ExecutionPlanContext& /*context*/) const override
    {
        auto it = m_literals.find(ref.key);
        if (it != m_literals.end()) {
            return it->second;
        }
        return std::nullopt;
    }

private:
    std::unordered_map<std::string, NetDiscovery::Plan::ExecutionValue> m_literals;
};

} // namespace Expressions
} // namespace NetDiscovery
