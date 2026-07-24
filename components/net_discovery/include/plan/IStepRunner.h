/**
 * @file IStepRunner.h
 * @brief Polymorphic step runner interface receiving StepExecutionState and IBindingResolver (v6.0 Phase D).
 */

#pragma once

#include "plan/IExecutionStep.h"
#include "plan/StepExecutionState.h"
#include "plan/ExecutionOutcome.h"
#include "plan/ExecutionPlanContext.h"
#include "plan/binding/IBindingResolver.h"
#include "plan/CancellationToken.h"
#include "ExecutionInfrastructure.h"

namespace NetDiscovery {
namespace Plan {

class IStepRunner {
public:
    virtual ~IStepRunner() = default;

    virtual ExecutionOutcome RunStep(IExecutionStep&          step,
                                     StepExecutionState&      state,
                                     ExecutionPlanContext&     context,
                                     ExecutionInfrastructure& infrastructure,
                                     const IBindingResolver&  bindingResolver,
                                     CancellationToken        cancelToken) = 0;
};

} // namespace Plan
} // namespace NetDiscovery
