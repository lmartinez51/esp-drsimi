/**
 * @file IVariableResolver.h
 * @brief Interface for resolving VariableRef against an execution context (v6.0 Phase D).
 *
 * IVariableResolver is a platform-level service interface. Implementations may resolve
 * variables from a blackboard, literal constants, remote sources, or chained resolvers.
 * The expression engine depends only on this interface — never on any concrete store.
 */

#pragma once

#include "expressions/VariableRef.h"
#include "plan/ExecutionValue.h"
#include "plan/ExecutionPlanContext.h"
#include <optional>

namespace NetDiscovery {
namespace Expressions {

class IVariableResolver {
public:
    virtual ~IVariableResolver() = default;

    /// Resolves a VariableRef against the given execution context.
    /// Returns std::nullopt if the variable is not present or its type does not match.
    virtual std::optional<NetDiscovery::Plan::ExecutionValue> Resolve(
        const VariableRef&                          ref,
        const NetDiscovery::Plan::ExecutionPlanContext& context) const = 0;
};

} // namespace Expressions
} // namespace NetDiscovery
