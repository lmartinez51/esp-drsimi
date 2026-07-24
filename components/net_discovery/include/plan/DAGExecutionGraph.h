/**
 * @file DAGExecutionGraph.h
 * @brief Directed Acyclic Graph implementation of IExecutionGraph (v6.0 Phase C).
 */

#pragma once

#include "plan/IExecutionGraph.h"
#include <unordered_map>

namespace NetDiscovery {
namespace Plan {

class DAGExecutionGraph : public IExecutionGraph {
public:
    DAGExecutionGraph() = default;

    void AddNode(std::shared_ptr<ExecutionNode> node) override;
    void AddEdge(const ExecutionEdge& edge) override;

    std::vector<std::shared_ptr<ExecutionNode>> GetNodes() const override;
    std::vector<ExecutionEdge> GetEdges() const override;
    std::shared_ptr<ExecutionNode> FindNode(const std::string& nodeId) const override;
    std::vector<std::shared_ptr<ExecutionNode>> GetInitialNodes() const override;
    std::vector<std::shared_ptr<ExecutionNode>> GetNextNodes(const std::string& nodeId) const override;

private:
    std::vector<std::shared_ptr<ExecutionNode>> m_orderedNodes;
    std::unordered_map<std::string, std::shared_ptr<ExecutionNode>> m_nodeMap;
    std::vector<ExecutionEdge> m_edges;
};

} // namespace Plan
} // namespace NetDiscovery
