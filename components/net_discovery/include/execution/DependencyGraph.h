/**
 * @file DependencyGraph.h
 * @brief Immutable DAG graph value object representing execution step dependencies (v5.0.0 Architecture Phase 8.6).
 */

#pragma once

#include "execution/ExecutionPlannerTypes.h"

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <queue>

namespace NetDiscovery {
namespace Execution {

/**
 * @brief Immutable value object representing execution step dependency graphs.
 */
class DependencyGraph {
public:
    DependencyGraph() = default;

    /**
     * @brief Constructor mapping stepId to list of stepIds it depends on (child dependencies).
     */
    explicit DependencyGraph(std::unordered_map<StepId, std::vector<StepId>> dependencies)
        : m_dependencies(std::move(dependencies)) {
        // Build reverse parent map (parents[dep] contains steps that depend on dep)
        for (const auto& [stepId, depList] : m_dependencies) {
            for (const auto& depId : depList) {
                m_parents[depId].push_back(stepId);
            }
        }
    }

    const std::unordered_map<StepId, std::vector<StepId>>& GetRawDependencies() const {
        return m_dependencies;
    }

    /**
     * @brief Returns direct dependencies (children) that target stepId depends on.
     */
    std::vector<StepId> GetDependencies(const StepId& stepId) const {
        auto it = m_dependencies.find(stepId);
        if (it != m_dependencies.end()) {
            return it->second;
        }
        return {};
    }

    /**
     * @brief Returns steps that depend on target stepId (parents).
     */
    std::vector<StepId> GetParents(const StepId& stepId) const {
        auto it = m_parents.find(stepId);
        if (it != m_parents.end()) {
            return it->second;
        }
        return {};
    }

    /**
     * @brief Root steps have zero dependencies and can execute immediately.
     */
    std::vector<StepId> GetRootSteps(const std::vector<StepId>& allStepIds) const {
        std::vector<StepId> roots;
        for (const auto& sId : allStepIds) {
            auto it = m_dependencies.find(sId);
            if (it == m_dependencies.end() || it->second.empty()) {
                roots.push_back(sId);
            }
        }
        return roots;
    }

    /**
     * @brief Leaf steps are final steps that no other step depends on.
     */
    std::vector<StepId> GetLeafSteps(const std::vector<StepId>& allStepIds) const {
        std::vector<StepId> leaves;
        for (const auto& sId : allStepIds) {
            auto it = m_parents.find(sId);
            if (it == m_parents.end() || it->second.empty()) {
                leaves.push_back(sId);
            }
        }
        return leaves;
    }

    /**
     * @brief Detects dependency cycles using Kahn's algorithm / in-degree check.
     */
    bool HasCycle(const std::vector<StepId>& allStepIds) const {
        std::unordered_map<StepId, size_t> inDegree;
        for (const auto& sId : allStepIds) {
            inDegree[sId] = 0;
        }
        for (const auto& sId : allStepIds) {
            auto deps = GetDependencies(sId);
            inDegree[sId] = deps.size();
        }

        std::queue<StepId> q;
        for (const auto& [sId, deg] : inDegree) {
            if (deg == 0) q.push(sId);
        }

        size_t visitedCount = 0;
        while (!q.empty()) {
            StepId curr = q.front();
            q.pop();
            visitedCount++;

            for (const auto& parent : GetParents(curr)) {
                if (inDegree.find(parent) != inDegree.end()) {
                    inDegree[parent]--;
                    if (inDegree[parent] == 0) {
                        q.push(parent);
                    }
                }
            }
        }

        return visitedCount != allStepIds.size();
    }

    /**
     * @brief Computes a topological execution ordering for steps.
     */
    std::vector<StepId> GetTopologicalSort(const std::vector<StepId>& allStepIds) const {
        std::unordered_map<StepId, size_t> inDegree;
        for (const auto& sId : allStepIds) {
            inDegree[sId] = 0;
        }
        for (const auto& sId : allStepIds) {
            inDegree[sId] = GetDependencies(sId).size();
        }

        std::queue<StepId> q;
        for (const auto& sId : allStepIds) {
            if (inDegree[sId] == 0) q.push(sId);
        }

        std::vector<StepId> sorted;
        sorted.reserve(allStepIds.size());

        while (!q.empty()) {
            StepId curr = q.front();
            q.pop();
            sorted.push_back(curr);

            for (const auto& parent : GetParents(curr)) {
                if (inDegree.find(parent) != inDegree.end()) {
                    inDegree[parent]--;
                    if (inDegree[parent] == 0) {
                        q.push(parent);
                    }
                }
            }
        }

        return sorted;
    }

private:
    std::unordered_map<StepId, std::vector<StepId>> m_dependencies;
    std::unordered_map<StepId, std::vector<StepId>> m_parents;
};

} // namespace Execution
} // namespace NetDiscovery
