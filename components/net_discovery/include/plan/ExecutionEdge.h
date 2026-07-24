/**
 * @file ExecutionEdge.h
 * @brief Directed edge between ExecutionNodes with condition semantics (v6.0 Phase C).
 */

#pragma once

#include <string>

namespace NetDiscovery {
namespace Plan {

enum class ExecutionEdgeType {
    Always,
    OnSuccess,
    OnFailure,
    OnCondition
};

struct ExecutionEdge {
    std::string sourceNodeId;
    std::string targetNodeId;
    ExecutionEdgeType type{ExecutionEdgeType::Always};
    std::string conditionKey;
};

} // namespace Plan
} // namespace NetDiscovery
