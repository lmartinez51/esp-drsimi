/**
 * @file ExecutionPlanExecutor.h
 * @brief Pure plan coordinator orchestrating lifecycle, observers, rollback, and cancellation (v6.0 Phase C.5).
 */

#pragma once

#include "plan/ExecutionPlanInstance.h"
#include "plan/ExecutionScheduler.h"
#include "plan/ExecutionPlanVerifier.h"
#include "plan/IRuntimeClock.h"
#include "plan/DefaultRuntimeClock.h"
#include "plan/IExecutionPlanObserver.h"
#include "plan/IRollbackCapable.h"
#include "ExecutionInfrastructure.h"
#include <memory>
#include <vector>

namespace NetDiscovery {
namespace Plan {

/**
 * @brief Coordinates ExecutionPlanInstance execution, verifies plans via ExecutionPlanVerifier, and notifies registered observers.
 */
class ExecutionPlanExecutor {
public:
    ExecutionPlanExecutor(std::shared_ptr<ExecutionInfrastructure> infrastructure,
                          std::shared_ptr<ExecutionPlanVerifier> verifier = nullptr,
                          std::shared_ptr<IRuntimeClock> clock = nullptr);

    void RegisterObserver(std::shared_ptr<IExecutionPlanObserver> observer);
    void UnregisterObserver(std::shared_ptr<IExecutionPlanObserver> observer);

    ExecutionResult ExecutePlan(ExecutionPlanInstance& instance);

private:
    void NotifyObservers(const ExecutionEvent& event);
    void PerformRollback(ExecutionPlanInstance& instance, const std::vector<std::shared_ptr<IExecutionStep>>& executedSteps);

    std::shared_ptr<ExecutionInfrastructure> m_infrastructure;
    std::shared_ptr<ExecutionPlanVerifier> m_verifier;
    std::shared_ptr<IRuntimeClock> m_clock;
    ExecutionScheduler m_scheduler;
    std::vector<std::shared_ptr<IExecutionPlanObserver>> m_observers;
};

} // namespace Plan
} // namespace NetDiscovery
