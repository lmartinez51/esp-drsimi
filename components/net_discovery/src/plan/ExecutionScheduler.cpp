/**
 * @file ExecutionScheduler.cpp
 * @brief Implementation of ExecutionScheduler (v6.0 Phase C Refinement 1).
 */

#include "plan/ExecutionScheduler.h"

namespace NetDiscovery {
namespace Plan {

std::vector<std::shared_ptr<ExecutionNode>> ExecutionScheduler::GetNextRunnableNodes(const ExecutionPlanInstance& instance) {
    std::vector<std::shared_ptr<ExecutionNode>> runnable;

    auto plan = instance.GetPlan();
    if (!plan || !plan->GetGraph()) {
        return runnable;
    }

    auto graph = plan->GetGraph();
    auto nodes = graph->GetNodes();

    for (const auto& node : nodes) {
        std::string id = node->GetNodeId();
        if (instance.GetStepState(id) == StepState::Pending) {
            if (AreDependenciesSatisfied(instance, id)) {
                runnable.push_back(node);
            }
        }
    }

    // Determinism specification §8: sort runnable candidates by NodeId
    std::sort(runnable.begin(), runnable.end(), [](const std::shared_ptr<ExecutionNode>& a, const std::shared_ptr<ExecutionNode>& b) {
        return a->GetNodeId() < b->GetNodeId();
    });

    return runnable;
}

bool ExecutionScheduler::AreDependenciesSatisfied(const ExecutionPlanInstance& instance, const std::string& nodeId) {
    auto plan = instance.GetPlan();
    if (!plan || !plan->GetGraph()) return true;

    auto graph = plan->GetGraph();
    auto edges = graph->GetEdges();

    for (const auto& edge : edges) {
        if (edge.targetNodeId == nodeId) {
            StepState sourceState = instance.GetStepState(edge.sourceNodeId);

            if (edge.type == ExecutionEdgeType::Always) {
                if (sourceState != StepState::Succeeded && sourceState != StepState::Skipped && sourceState != StepState::Failed) {
                    return false;
                }
            } else if (edge.type == ExecutionEdgeType::OnSuccess) {
                if (sourceState != StepState::Succeeded) {
                    return false;
                }
            } else if (edge.type == ExecutionEdgeType::OnFailure) {
                if (sourceState != StepState::Failed) {
                    return false;
                }
            }
        }
    }

    return true;
}

} // namespace Plan
} // namespace NetDiscovery
