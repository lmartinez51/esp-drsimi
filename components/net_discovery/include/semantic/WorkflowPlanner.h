#pragma once

#include "WorkflowTemplate.h"
#include "../core/ActionId.h"
#include "semantic/SemanticDataModels.h"

namespace semantic {

class WorkflowPlanner {
public:
    /**
     * @brief Expands a canonical intent into an ExecutionPlan based on declarative templates.
     * @param intent The intent to fulfill.
     * @param target The target device.
     * @param parameters Normalized parameters for the actions.
     * @return The generated ExecutionPlan, or an empty plan if no template exists.
     */
    ExecutionPlan CreatePlan(NetDiscovery::ActionId intent, 
                             const NetDiscovery::LogicalDevice& target,
                             const std::map<std::string, ExecutionParameterVariant>& parameters) const;

private:
    WorkflowTemplate GetTemplateForIntent(NetDiscovery::ActionId intent) const;
};

} // namespace semantic
