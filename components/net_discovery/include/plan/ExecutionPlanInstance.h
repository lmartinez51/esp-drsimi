/**
 * @file ExecutionPlanInstance.h
 * @brief Encapsulates transient runtime execution state for an IExecutionPlan (v6.0 Phase C).
 */

#pragma once

#include "plan/IExecutionPlan.h"
#include "plan/ExecutionState.h"
#include "plan/StepExecutionState.h"
#include "plan/ExecutionPlanContext.h"
#include "plan/CancellationToken.h"
#include "plan/ExecutionProgress.h"
#include <unordered_map>
#include <mutex>
#include <memory>
#include <chrono>

namespace NetDiscovery {
namespace Plan {

class ExecutionPlanInstance {
public:
    ExecutionPlanInstance(std::string instanceId,
                          std::shared_ptr<IExecutionPlan> plan,
                          CancellationToken cancelToken = CancellationToken());

    std::string GetInstanceId() const { return m_instanceId; }
    std::shared_ptr<IExecutionPlan> GetPlan() const { return m_plan; }

    PlanState GetState() const;
    void SetState(PlanState state);

    StepState GetStepState(const std::string& stepId) const;
    void SetStepState(const std::string& stepId, StepState state);

    struct StepExecutionState& GetStepExecutionState(const std::string& stepId);
    const struct StepExecutionState& GetStepExecutionState(const std::string& stepId) const;

    ExecutionPlanContext& GetContext() { return m_context; }
    const ExecutionPlanContext& GetContext() const { return m_context; }

    CancellationToken GetCancellationToken() const { return m_cancelToken; }
    ExecutionProgress GetProgress() const;

    uint64_t GetStartTimeMs() const { return m_startTimeMs; }
    uint64_t GetEndTimeMs() const { return m_endTimeMs; }

private:
    std::string m_instanceId;
    std::shared_ptr<IExecutionPlan> m_plan;
    PlanState m_state{PlanState::Pending};
    std::unordered_map<std::string, StepState> m_stepStates;
    std::unordered_map<std::string, StepExecutionState> m_fullStepStates;
    ExecutionPlanContext m_context;
    CancellationToken m_cancelToken;
    uint64_t m_startTimeMs{0};
    uint64_t m_endTimeMs{0};
    mutable std::mutex m_mutex;
};

} // namespace Plan
} // namespace NetDiscovery
