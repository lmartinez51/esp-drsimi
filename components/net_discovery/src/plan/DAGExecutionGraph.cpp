/**
 * @file DAGExecutionGraph.cpp
 * @brief Implementation of DAGExecutionGraph (v6.0 Phase C).
 */

#include "plan/DAGExecutionGraph.h"
#include <algorithm>

namespace NetDiscovery {
namespace Plan {

void DAGExecutionGraph::AddNode(std::shared_ptr<ExecutionNode> node) {
    if (!node) return;
    std::string id = node->GetNodeId();
    if (m_nodeMap.find(id) == m_nodeMap.end()) {
        m_nodeMap[id] = node;
        m_orderedNodes.push_back(node);
    }
}

void DAGExecutionGraph::AddEdge(const ExecutionEdge& edge) {
    m_edges.push_back(edge);
}

std::vector<std::shared_ptr<ExecutionNode>> DAGExecutionGraph::GetNodes() const {
    return m_orderedNodes;
}

std::vector<ExecutionEdge> DAGExecutionGraph::GetEdges() const {
    return m_edges;
}

std::shared_ptr<ExecutionNode> DAGExecutionGraph::FindNode(const std::string& nodeId) const {
    auto it = m_nodeMap.find(nodeId);
    if (it != m_nodeMap.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<std::shared_ptr<ExecutionNode>> DAGExecutionGraph::GetInitialNodes() const {
    std::vector<std::shared_ptr<ExecutionNode>> initial;
    std::unordered_map<std::string, bool> hasIncoming;

    for (const auto& edge : m_edges) {
        hasIncoming[edge.targetNodeId] = true;
    }

    for (const auto& node : m_orderedNodes) {
        if (!hasIncoming[node->GetNodeId()]) {
            initial.push_back(node);
        }
    }

    if (initial.empty() && !m_orderedNodes.empty()) {
        initial.push_back(m_orderedNodes.front());
    }

    return initial;
}

std::vector<std::shared_ptr<ExecutionNode>> DAGExecutionGraph::GetNextNodes(const std::string& nodeId) const {
    std::vector<std::shared_ptr<ExecutionNode>> nextNodes;
    for (const auto& edge : m_edges) {
        if (edge.sourceNodeId == nodeId) {
            auto target = FindNode(edge.targetNodeId);
            if (target) {
                nextNodes.push_back(target);
            }
        }
    }
    return nextNodes;
}

} // namespace Plan
} // namespace NetDiscovery
