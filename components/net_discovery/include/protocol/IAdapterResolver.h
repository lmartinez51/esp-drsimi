/**
 * @file IAdapterResolver.h
 * @brief Pure abstract interface for adapter resolution (v5.0.0 Architecture Phase 10.1).
 */

#pragma once

#include "protocol/AdapterResolutionResult.h"
#include "execution/ExecutionStep.h"
#include "binding/ActionBinding.h"
#include "runtime/ExecutionRuntimeContext.h"

namespace NetDiscovery {
namespace Protocol {

/**
 * @brief Pure abstract interface for adapter resolution.
 *
 * Runtime execution infrastructure depends exclusively on this interface.
 * Neither RuntimeExecutionEngine nor ProtocolAdapterDispatcher queries registries or factories directly.
 */
class IAdapterResolver {
public:
    virtual ~IAdapterResolver() = default;

    /**
     * @brief Resolves an executable step to a concrete ResolvedAdapter.
     */
    virtual AdapterResolutionResult Resolve(
        const Execution::ExecutionStep&        step,
        const Runtime::ExecutionRuntimeContext& context) = 0;

    /**
     * @brief Resolves an executable step and ActionBinding to a concrete ResolvedAdapter.
     */
    virtual AdapterResolutionResult Resolve(
        const Execution::ExecutionStep&        step,
        const Binding::ActionBinding&          binding,
        const Runtime::ExecutionRuntimeContext& context) = 0;
};

} // namespace Protocol
} // namespace NetDiscovery
