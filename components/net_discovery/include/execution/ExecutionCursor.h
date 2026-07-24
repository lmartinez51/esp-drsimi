/**
 * @file ExecutionCursor.h
 * @brief Traversal cursor traversing DependencyGraph step progress (v5.0.0 Architecture Phase 9).
 */

#pragma once

#include "execution/ExecutionPlan.h"
#include "execution/ExecutionStep.h"

#include <vector>
#include <unordered_set>
#include <optional>
#include <algorithm>

namespace NetDiscovery {
namespace Execution {

/**
 * @brief Traversal mechanism tracking completed, failed, and remaining steps in an ExecutionPlan graph.
 */
class ExecutionCursor {
public:
    ExecutionCursor() = default;

    explicit ExecutionCursor(const ExecutionPlan* plan)
        : m_plan(plan) {
        if (m_plan) {
            for (const auto& step : m_plan->GetSteps()) {
                m_pendingSteps.insert(step.GetStepId());
            }
        }
    }

    const ExecutionPlan* GetPlan() const { return m_plan; }

    std::optional<ExecutionStep> CurrentStep() const {
        if (!m_plan || m_currentStepId.empty()) return std::nullopt;
        for (const auto& step : m_plan->GetSteps()) {
            if (step.GetStepId() == m_currentStepId) return step;
        }
        return std::nullopt;
    }

    bool HasNext() const {
        return !m_pendingSteps.empty();
    }

    std::vector<StepId> GetReadySteps() const {
        if (!m_plan) return {};
        
        std::vector<StepId> ready;
        const auto& depGraph = m_plan->GetDependencyGraph();

        for (const auto& stepId : m_pendingSteps) {
            auto parents = depGraph.GetParents(stepId);
            bool allParentsCompleted = true;
            for (const auto& parentId : parents) {
                if (m_completedSteps.find(parentId) == m_completedSteps.end() &&
                    m_skippedSteps.find(parentId) == m_skippedSteps.end()) {
                    allParentsCompleted = false;
                    break;
                }
            }
            if (allParentsCompleted) {
                ready.push_back(stepId);
            }
        }

        return ready;
    }

    void Advance(const StepId& nextStepId) {
        m_currentStepId = nextStepId;
    }

    void MarkCompleted(const StepId& stepId) {
        m_pendingSteps.erase(stepId);
        m_completedSteps.insert(stepId);
        if (m_currentStepId == stepId) {
            m_currentStepId.clear();
        }
    }

    void MarkFailed(const StepId& stepId) {
        m_pendingSteps.erase(stepId);
        m_failedSteps.insert(stepId);
        if (m_currentStepId == stepId) {
            m_currentStepId.clear();
        }
    }

    void SkipStep(const StepId& stepId) {
        m_pendingSteps.erase(stepId);
        m_skippedSteps.insert(stepId);
        if (m_currentStepId == stepId) {
            m_currentStepId.clear();
        }
    }

    const std::unordered_set<StepId>& GetCompletedSteps() const { return m_completedSteps; }
    const std::unordered_set<StepId>& GetFailedSteps() const { return m_failedSteps; }
    const std::unordered_set<StepId>& GetPendingSteps() const { return m_pendingSteps; }

private:
    const ExecutionPlan* m_plan{nullptr};
    StepId m_currentStepId;
    std::unordered_set<StepId> m_pendingSteps;
    std::unordered_set<StepId> m_completedSteps;
    std::unordered_set<StepId> m_failedSteps;
    std::unordered_set<StepId> m_skippedSteps;
};

} // namespace Execution
} // namespace NetDiscovery
