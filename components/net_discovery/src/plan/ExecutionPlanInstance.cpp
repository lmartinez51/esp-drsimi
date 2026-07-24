/**
 * @file ExecutionPlanInstance.cpp
 * @brief Implementation of ExecutionPlanInstance (v6.0 Phase C).
 */

#include "plan/ExecutionPlanInstance.h"
#include "esp_timer.h"

namespace NetDiscovery {
namespace Plan {

ExecutionPlanInstance::ExecutionPlanInstance(std::string instanceId,
                                               std::shared_ptr<IExecutionPlan> plan,
                                               CancellationToken cancelToken)
    : m_instanceId(std::move(instanceId))
    , m_plan(std::move(plan))
    , m_cancelToken(std::move(cancelToken))
{
    m_startTimeMs = static_cast<uint64_t>(esp_timer_get_time() / 1000);
}

PlanState ExecutionPlanInstance::GetState() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_state;
}

void ExecutionPlanInstance::SetState(PlanState state) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_state = state;
    if (state == PlanState::Completed || state == PlanState::Failed || state == PlanState::Cancelled || state == PlanState::RolledBack) {
        m_endTimeMs = static_cast<uint64_t>(esp_timer_get_time() / 1000);
    }
}

StepState ExecutionPlanInstance::GetStepState(const std::string& stepId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_stepStates.find(stepId);
    if (it != m_stepStates.end()) {
        return it->second;
    }
    return StepState::Pending;
}

void ExecutionPlanInstance::SetStepState(const std::string& stepId, StepState state) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stepStates[stepId] = state;
    m_fullStepStates[stepId].stepId = stepId;
    m_fullStepStates[stepId].state = state;
}

StepExecutionState& ExecutionPlanInstance::GetStepExecutionState(const std::string& stepId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto& s = m_fullStepStates[stepId];
    s.stepId = stepId;
    return s;
}

const StepExecutionState& ExecutionPlanInstance::GetStepExecutionState(const std::string& stepId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_fullStepStates.find(stepId);
    if (it != m_fullStepStates.end()) {
        return it->second;
    }
    static const StepExecutionState s_emptyState;
    return s_emptyState;
}

ExecutionProgress ExecutionPlanInstance::GetProgress() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    ExecutionProgress progress;
    progress.state = m_state;

    if (!m_plan || !m_plan->GetGraph()) {
        return progress;
    }

    auto nodes = m_plan->GetGraph()->GetNodes();
    progress.totalSteps = static_cast<int>(nodes.size());

    int completed = 0;
    for (const auto& node : nodes) {
        auto id = node->GetNodeId();
        auto it = m_stepStates.find(id);
        if (it != m_stepStates.end()) {
            if (it->second == StepState::Succeeded || it->second == StepState::Skipped) {
                completed++;
            }
        }
    }

    progress.completedSteps = completed;
    if (progress.totalSteps > 0) {
        progress.percentage = (static_cast<float>(completed) / static_cast<float>(progress.totalSteps)) * 100.0f;
    }

    uint64_t nowMs = static_cast<uint64_t>(esp_timer_get_time() / 1000);
    progress.elapsedTimeMs = nowMs - m_startTimeMs;

    return progress;
}

} // namespace Plan
} // namespace NetDiscovery
