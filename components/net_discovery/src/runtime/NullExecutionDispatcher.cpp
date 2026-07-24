/**
 * @file NullExecutionDispatcher.cpp
 * @brief Implementation of NullExecutionDispatcher (v5.0.0 Architecture Phase 9.2).
 */

#include "runtime/NullExecutionDispatcher.h"

namespace NetDiscovery {
namespace Runtime {

ExecutionStepResult NullExecutionDispatcher::Dispatch(
        const Execution::ExecutionStep&  step,
        Execution::ExecutionSession&     session,
        ExecutionRuntimeContext&         context) {
    (void)session;
    (void)context;
    return ExecutionStepResult(step.GetStepId(),
                               Execution::StepStatus::NotImplemented,
                               step.GetAdapterId(),
                               0, 0, 0,
                               "Protocol adapter not attached (Phase 9.2 null dispatcher stub)",
                               false, false, {}, {},
                               {{"BindingId", step.GetBindingId()}});
}

DispatcherCapabilities NullExecutionDispatcher::GetCapabilities() const {
    return DispatcherCapabilities::None();
}

} // namespace Runtime
} // namespace NetDiscovery
