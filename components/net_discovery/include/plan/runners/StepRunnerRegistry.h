/**
 * @file StepRunnerRegistry.h
 * @brief Registration-based StepRunnerRegistry replacing factory switch (v6.0 Phase D).
 */

#pragma once

#include "plan/IStepRunner.h"
#include <memory>
#include <functional>
#include <unordered_map>

namespace NetDiscovery {
namespace Plan {

using RunnerFactoryFn = std::function<std::shared_ptr<IStepRunner>(IExecutionStep&)>;

class StepRunnerRegistry {
public:
    StepRunnerRegistry();

    void Register(StepType type, RunnerFactoryFn factory);
    std::shared_ptr<IStepRunner> CreateRunner(IExecutionStep& step) const;

    static StepRunnerRegistry& Instance();

private:
    std::unordered_map<StepType, RunnerFactoryFn> m_factories;
};

} // namespace Plan
} // namespace NetDiscovery
