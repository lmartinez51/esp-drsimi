/**
 * @file ExecutionScheduler.h
 * @brief Graph traversal, dependency satisfaction, and runnable node scheduler (v6.0 Phase C Refinement 1).
 */

#pragma once

#include "plan/ExecutionPlanInstance.h"
#include "plan/ExecutionNode.h"
#include <memory>
#include <vector>

namespace NetDiscovery {
namespace Plan {

class ExecutionScheduler {
public:
    ExecutionScheduler() = default;

    /**
     * @brief Evaluates current plan instance state and returns the next batch of ready nodes to execute.
     */
    std::vector<std::shared_ptr<ExecutionNode>> GetNextRunnableNodes(const ExecutionPlanInstance& instance);

    /**
     * @brief Checks if all dependencies for a target node are satisfied in the current instance state.
     */
    bool AreDependenciesSatisfied(const ExecutionPlanInstance& instance, const std::string& nodeId);
};

} // namespace Plan
} // namespace NetDiscovery
