/**
 * @file NullExecutionDispatcher.h
 * @brief Default null implementation returning StepStatus::NotImplemented (v5.0.0 Architecture Phase 9.2).
 */

#pragma once

#include "runtime/IExecutionDispatcher.h"

namespace NetDiscovery {
namespace Runtime {

/**
 * @brief Null object implementation of IExecutionDispatcher.
 *
 * Returns NotImplemented for every step. Exposes DispatcherCapabilities::None().
 * Serves as the default dispatcher until a real protocol adapter is injected.
 */
class NullExecutionDispatcher : public IExecutionDispatcher {
public:
    NullExecutionDispatcher() = default;
    ~NullExecutionDispatcher() override = default;

    ExecutionStepResult Dispatch(const Execution::ExecutionStep&  step,
                                  Execution::ExecutionSession&     session,
                                  ExecutionRuntimeContext&         context) override;

    DispatcherCapabilities GetCapabilities() const override;
};

} // namespace Runtime
} // namespace NetDiscovery
