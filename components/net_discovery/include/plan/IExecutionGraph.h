/**
 * @file IExecutionGraph.h
 * @brief Abstract graph interface for workflow graph traversal (v6.0 Phase C).
 */

#pragma once

#include "plan/ExecutionNode.h"
#include "plan/ExecutionEdge.h"
#include <vector>
#include <memory>

namespace NetDiscovery {
namespace Plan {

class IExecutionGraph {
public:
    virtual ~IExecutionGraph() = default;

    virtual void AddNode(std::shared_ptr<ExecutionNode> node) = 0;
    virtual void AddEdge(const ExecutionEdge& edge) = 0;

    virtual std::vector<std::shared_ptr<ExecutionNode>> GetNodes() const = 0;
    virtual std::vector<ExecutionEdge> GetEdges() const = 0;
    virtual std::shared_ptr<ExecutionNode> FindNode(const std::string& nodeId) const = 0;
    virtual std::vector<std::shared_ptr<ExecutionNode>> GetInitialNodes() const = 0;
    virtual std::vector<std::shared_ptr<ExecutionNode>> GetNextNodes(const std::string& nodeId) const = 0;
};

} // namespace Plan
} // namespace NetDiscovery
