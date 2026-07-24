/**
 * @file IExecutionPlan.h
 * @brief Pure interface representing an immutable workflow blueprint (v6.0 Phase C).
 */

#pragma once

#include "plan/IExecutionGraph.h"
#include "core/ExecutionPolicy.h"
#include <string>
#include <unordered_map>
#include <memory>

namespace NetDiscovery {
namespace Plan {

class IExecutionPlan {
public:
    virtual ~IExecutionPlan() = default;

    virtual std::string GetPlanId() const = 0;
    virtual std::string GetPlanName() const = 0;
    virtual const std::unordered_map<std::string, std::string>& GetMetadata() const = 0;
    virtual const NetDiscovery::ExecutionPolicy& GetPolicy() const = 0;
    virtual std::shared_ptr<IExecutionGraph> GetGraph() const = 0;
};

} // namespace Plan
} // namespace NetDiscovery
