#pragma once

#include <string>
#include <vector>
#include <chrono>
#include "core/Capability.h"
#include "core/ActionId.h"
#include "semantic/SemanticDataModels.h"

namespace semantic {

/**
 * @brief Represents a declarative macro/recipe for fulfilling a ActionId.
 * It contains NO executable logic, only structural data.
 */
struct WorkflowTemplate {
    NetDiscovery::ActionId intent;
    NetDiscovery::Capability requiredCapability;

    struct TemplateStep {
        NetDiscovery::ActionId actionId;
        bool isOptional;
        ExecutionWaitPolicy waitPolicy;
        std::chrono::milliseconds waitTimeout;
        RetryPolicy retryPolicy;
    };

    std::vector<TemplateStep> steps;
};

} // namespace semantic
