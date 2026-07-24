/**
 * @file ExecutionDispatcher.h
 * @brief Dispatcher extension point for step dispatch (v5.0.0 Architecture Phase 9).
 */

#pragma once

#include "execution/ExecutionStep.h"
#include "execution/ExecutionStepResult.h"

namespace NetDiscovery {
namespace Execution {

/**
 * @brief Dispatcher component routing ExecutionSteps to concrete protocol adapters (Phase 10 extension point).
 */
class ExecutionDispatcher {
public:
    ExecutionDispatcher() = default;
    virtual ~ExecutionDispatcher() = default;

    /**
     * @brief Dispatches an ExecutionStep. In Phase 9, returns StepStatus::NotImplemented.
     */
    virtual ExecutionStepResult Dispatch(const ExecutionStep& step);
};

} // namespace Execution
} // namespace NetDiscovery
