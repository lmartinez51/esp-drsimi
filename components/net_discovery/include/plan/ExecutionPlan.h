/**
 * @file ExecutionPlan.h
 * @brief Concrete immutable implementation of IExecutionPlan blueprint (v6.0 Phase C).
 */

#pragma once

#include "plan/IExecutionPlan.h"
#include "plan/DAGExecutionGraph.h"

namespace NetDiscovery {
namespace Plan {

class ExecutionPlan : public IExecutionPlan {
public:
    ExecutionPlan(std::string planId,
                  std::string planName,
                  std::shared_ptr<IExecutionGraph> graph = std::make_shared<DAGExecutionGraph>(),
                  NetDiscovery::ExecutionPolicy policy = NetDiscovery::ExecutionPolicy::Interactive(),
                  std::unordered_map<std::string, std::string> metadata = {})
        : m_planId(std::move(planId))
        , m_planName(std::move(planName))
        , m_graph(std::move(graph))
        , m_policy(policy)
        , m_metadata(std::move(metadata)) {}

    std::string GetPlanId() const override { return m_planId; }
    std::string GetPlanName() const override { return m_planName; }
    const std::unordered_map<std::string, std::string>& GetMetadata() const override { return m_metadata; }
    const NetDiscovery::ExecutionPolicy& GetPolicy() const override { return m_policy; }
    std::shared_ptr<IExecutionGraph> GetGraph() const override { return m_graph; }

private:
    std::string m_planId;
    std::string m_planName;
    std::shared_ptr<IExecutionGraph> m_graph;
    NetDiscovery::ExecutionPolicy m_policy;
    std::unordered_map<std::string, std::string> m_metadata;
};

} // namespace Plan
} // namespace NetDiscovery
