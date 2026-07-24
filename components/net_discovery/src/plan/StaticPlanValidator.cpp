/**
 * @file StaticPlanValidator.cpp
 * @brief Implementation of StaticPlanValidator (v6.0 Phase C.5).
 */

#include "plan/StaticPlanValidator.h"
#include <unordered_map>
#include <unordered_set>
#include <queue>

namespace NetDiscovery {
namespace Plan {

ValidationReport StaticPlanValidator::ValidateGraph(const IExecutionGraph& graph) const {
    ValidationReport report;

    auto nodes = graph.GetNodes();
    auto edges = graph.GetEdges();

    if (nodes.empty()) {
        report.AddIssue(ValidationSeverity::Error, ValidationCode::EmptyGraph, "Graph", "ExecutionGraph contains no nodes.");
        return report;
    }

    // 1. Duplicate node ID check & non-empty ID check
    std::unordered_map<std::string, bool> nodeIds;
    for (const auto& node : nodes) {
        if (!node) {
            report.AddIssue(ValidationSeverity::Error, ValidationCode::NullStep, "Node", "Encountered null node in graph.");
            continue;
        }
        std::string id = node->GetNodeId();
        if (id.empty()) {
            report.AddIssue(ValidationSeverity::Error, ValidationCode::DuplicateNodeId, "Node", "Node has empty ID.");
        } else if (nodeIds.find(id) != nodeIds.end()) {
            report.AddIssue(ValidationSeverity::Error, ValidationCode::DuplicateNodeId, id, "Duplicate node ID found: " + id);
        } else {
            nodeIds[id] = true;
        }
    }

    // 2. Edge reference integrity check
    std::unordered_map<std::string, std::vector<std::string>> adj;
    std::unordered_map<std::string, int> inDegree;
    for (const auto& pair : nodeIds) {
        inDegree[pair.first] = 0;
    }

    for (const auto& edge : edges) {
        if (nodeIds.find(edge.sourceNodeId) == nodeIds.end()) {
            report.AddIssue(ValidationSeverity::Error, ValidationCode::InvalidEdgeReference, edge.sourceNodeId, "Edge references non-existent source node: " + edge.sourceNodeId);
        }
        if (nodeIds.find(edge.targetNodeId) == nodeIds.end()) {
            report.AddIssue(ValidationSeverity::Error, ValidationCode::InvalidEdgeReference, edge.targetNodeId, "Edge references non-existent target node: " + edge.targetNodeId);
        }
        if (nodeIds.find(edge.sourceNodeId) != nodeIds.end() && nodeIds.find(edge.targetNodeId) != nodeIds.end()) {
            adj[edge.sourceNodeId].push_back(edge.targetNodeId);
            inDegree[edge.targetNodeId]++;
        }
    }

    // 3. Entry node validation
    auto initialNodes = graph.GetInitialNodes();
    if (initialNodes.empty()) {
        report.AddIssue(ValidationSeverity::Error, ValidationCode::MissingEntryNode, "Graph", "No valid initial entry nodes found in graph.");
    }

    // 4. Dependency cycle detection (Kahn's Topological Sort Algorithm)
    std::queue<std::string> q;
    for (const auto& pair : inDegree) {
        if (pair.second == 0) {
            q.push(pair.first);
        }
    }

    int visitedCount = 0;
    std::unordered_set<std::string> reachable;

    while (!q.empty()) {
        std::string curr = q.front();
        q.pop();
        visitedCount++;
        reachable.insert(curr);

        auto it = adj.find(curr);
        if (it != adj.end()) {
            for (const auto& neighbor : it->second) {
                inDegree[neighbor]--;
                if (inDegree[neighbor] == 0) {
                    q.push(neighbor);
                }
            }
        }
    }

    if (visitedCount < static_cast<int>(nodeIds.size())) {
        report.AddIssue(ValidationSeverity::Error, ValidationCode::DependencyCycle, "Graph", "Dependency cycle detected in execution graph (DAG property violated).");
    }

    // 5. Orphan & Unreachable node detection
    if (nodes.size() > 1) {
        for (const auto& pair : nodeIds) {
            std::string id = pair.first;
            bool hasIn = false;
            bool hasOut = false;

            for (const auto& edge : edges) {
                if (edge.targetNodeId == id) hasIn = true;
                if (edge.sourceNodeId == id) hasOut = true;
            }

            if (!hasIn && !hasOut) {
                report.AddIssue(ValidationSeverity::Warning, ValidationCode::OrphanNode, id, "Orphan node detected with zero incoming and zero outgoing edges: " + id);
            }

            if (reachable.find(id) == reachable.end()) {
                report.AddIssue(ValidationSeverity::Error, ValidationCode::UnreachableNode, id, "Unreachable node detected: " + id);
            }
        }
    }

    return report;
}

} // namespace Plan
} // namespace NetDiscovery
