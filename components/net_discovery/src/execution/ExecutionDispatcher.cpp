/**
 * @file ExecutionDispatcher.cpp
 * @brief Implementation of ExecutionDispatcher (v5.0.0 Architecture Phase 9).
 */

#include "execution/ExecutionDispatcher.h"

namespace NetDiscovery {
namespace Execution {

ExecutionStepResult ExecutionDispatcher::Dispatch(const ExecutionStep& step) {
    // Phase 9 placeholder implementation: protocol adapters are introduced in Phase 10.
    return ExecutionStepResult(StepStatus::NotImplemented,
                              step.GetStepId(),
                              0,
                              "Protocol execution engine not attached (Phase 9 architecture layer only)",
                              {},
                              {{"AdapterId", step.GetAdapterId()}, {"BindingId", step.GetBindingId()}});
}

} // namespace Execution
} // namespace NetDiscovery
