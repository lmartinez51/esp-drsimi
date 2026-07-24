/**
 * @file ExecutionPlan.h
 * @brief Immutable graph of ExecutionSteps (v5.0.0 Architecture Phase 8.6).
 * 
 * ExecutionPlan represents a complete, immutable execution plan graph.
 * Owns PlanIdentifier, DependencyGraph, PlanStatistics, and ExecutionCapabilities.
 */

#pragma once

#include "execution/ExecutionStep.h"
#include "execution/ExecutionPolicy.h"
#include "execution/ExecutionPlannerTypes.h"
#include "execution/PlanIdentifier.h"
#include "execution/DependencyGraph.h"
#include "execution/PlanStatistics.h"
#include "execution/ExecutionCapabilities.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace NetDiscovery {
namespace Execution {

/**
 * @brief Immutable representation of a planned execution graph.
 */
class ExecutionPlan {
public:
    ExecutionPlan() = default;

    ExecutionPlan(PlanIdentifier planIdentifier,
                  std::vector<ExecutionStep> steps,
                  DependencyGraph dependencyGraph,
                  PlanStatistics statistics,
                  ExecutionCapabilities capabilities = {},
                  ExecutionPolicy executionPolicy = {},
                  PlanStatus status = PlanStatus::Pending,
                  std::unordered_map<std::string, std::string> metadata = {})
        : m_planIdentifier(std::move(planIdentifier)),
          m_steps(std::move(steps)),
          m_dependencyGraph(std::move(dependencyGraph)),
          m_statistics(std::move(statistics)),
          m_capabilities(std::move(capabilities)),
          m_executionPolicy(std::move(executionPolicy)),
          m_status(status),
          m_metadata(std::move(metadata)) {}

    // Legacy constructor compatibility
    ExecutionPlan(PlanId planId,
                  RequestId requestId,
                  std::vector<ExecutionStep> steps,
                  std::unordered_map<StepId, std::vector<StepId>> dependencies = {},
                  uint32_t estimatedDurationMs = 0,
                  ExecutionPolicy executionPolicy = {},
                  PlanStatus status = PlanStatus::Pending,
                  std::unordered_map<std::string, std::string> metadata = {})
        : m_planIdentifier(PlanIdentifier(std::move(requestId))),
          m_steps(std::move(steps)),
          m_dependencyGraph(DependencyGraph(std::move(dependencies))),
          m_executionPolicy(std::move(executionPolicy)),
          m_status(status),
          m_metadata(std::move(metadata)) {
        m_statistics.estimatedDurationMs = estimatedDurationMs;
        if (m_statistics.estimatedDurationMs == 0) {
            for (const auto& step : m_steps) {
                m_statistics.estimatedDurationMs += step.GetEstimatedDurationMs();
            }
        }
    }

    // Immutable Accessors
    const PlanIdentifier& GetPlanIdentifier() const { return m_planIdentifier; }
    std::string GetPlanId() const { return m_planIdentifier.ToString(); }
    const RequestId& GetRequestId() const { return m_planIdentifier.GetRequestId(); }
    const std::vector<ExecutionStep>& GetSteps() const { return m_steps; }
    const DependencyGraph& GetDependencyGraph() const { return m_dependencyGraph; }
    const std::unordered_map<StepId, std::vector<StepId>>& GetDependencies() const { return m_dependencyGraph.GetRawDependencies(); }
    const PlanStatistics& GetStatistics() const { return m_statistics; }
    const ExecutionCapabilities& GetCapabilities() const { return m_capabilities; }
    uint32_t GetEstimatedDurationMs() const { return m_statistics.estimatedDurationMs; }
    const ExecutionPolicy& GetExecutionPolicy() const { return m_executionPolicy; }
    PlanStatus GetStatus() const { return m_status; }
    const std::unordered_map<std::string, std::string>& GetMetadata() const { return m_metadata; }

    size_t GetStepCount() const { return m_steps.size(); }
    bool IsEmpty() const { return m_steps.empty(); }

    bool operator==(const ExecutionPlan& other) const {
        return m_planIdentifier == other.m_planIdentifier;
    }

private:
    PlanIdentifier m_planIdentifier;
    std::vector<ExecutionStep> m_steps;
    DependencyGraph m_dependencyGraph;
    PlanStatistics m_statistics;
    ExecutionCapabilities m_capabilities;
    ExecutionPolicy m_executionPolicy;
    PlanStatus m_status{PlanStatus::Pending};
    std::unordered_map<std::string, std::string> m_metadata;
};

} // namespace Execution
} // namespace NetDiscovery
