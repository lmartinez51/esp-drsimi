/**
 * @file StepRunnerFactory.h
 * @brief StepRunnerFactory forwarder delegating to StepRunnerRegistry (v6.0 Phase C / Phase D compatibility).
 */

#pragma once

#include "plan/IStepRunner.h"
#include "plan/runners/StepRunnerRegistry.h"
#include <memory>

namespace NetDiscovery {
namespace Plan {

class StepRunnerFactory {
public:
    static std::shared_ptr<IStepRunner> CreateRunner(IExecutionStep& step) {
        return StepRunnerRegistry::Instance().CreateRunner(step);
    }
};

} // namespace Plan
} // namespace NetDiscovery
