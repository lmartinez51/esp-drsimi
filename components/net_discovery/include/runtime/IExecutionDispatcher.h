/**
 * @file IExecutionDispatcher.h
 * @brief Frozen pure abstract interface for protocol adapter step dispatchers (v5.0.0 Architecture Phase 9.2).
 */

#pragma once

#include "execution/ExecutionStep.h"
#include "execution/ExecutionSession.h"
#include "runtime/ExecutionStepResult.h"
#include "runtime/ExecutionRuntimeContext.h"
#include "runtime/DispatcherCapabilities.h"

namespace NetDiscovery {
namespace Runtime {

/**
 * @brief Permanent architectural boundary interface for step dispatching.
 *
 * Phase 9.2 frozen API: all arguments required for execution are passed explicitly.
 * Dispatchers must NEVER query repositories or hold shared state.
 */
class IExecutionDispatcher {
public:
    virtual ~IExecutionDispatcher() = default;

    /**
     * @brief Dispatches a single ExecutionStep within the context of its owning session.
     *
     * @param step     Immutable step metadata describing the operation to perform.
     * @param session  Owning session providing plan identity and completed-step history.
     * @param context  Mutable runtime state scoped to the session (outputs, retries, flags).
     *
     * All inputs required for execution must be reachable through these three parameters.
     * Dispatchers must not consult external repositories or shared singletons.
     */
    virtual ExecutionStepResult Dispatch(const Execution::ExecutionStep&    step,
                                         Execution::ExecutionSession&        session,
                                         ExecutionRuntimeContext&            context) = 0;

    /**
     * @brief Returns the static capabilities this dispatcher exposes.
     *
     * Called once at session start by RuntimeExecutionEngine to validate plan compatibility.
     */
    virtual DispatcherCapabilities GetCapabilities() const = 0;
};

} // namespace Runtime
} // namespace NetDiscovery
