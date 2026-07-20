#pragma once

#include "semantic/SemanticDataModels.h"
#include "semantic/SemanticError.h"
#include "semantic/IntentCanonicalizer.h"
#include "semantic/DeviceMatcher.h"
#include "semantic/CapabilityFilter.h"
#include "semantic/ParameterNormalizer.h"
#include "semantic/LogicalEntityResolver.h"
#include "semantic/WorkflowPlanner.h"
#include "ExecutionEngine.h"
#include <memory>
#include <atomic>
#include <vector>

namespace semantic {

class SemanticOrchestrator {
public:
    SemanticOrchestrator(std::shared_ptr<NetDiscovery::ExecutionEngine> executionEngine);

    /**
     * @brief Resolves and orchestrates a semantic request into a physical execution plan.
     * @param request The abstract request.
     * @param availableDevices The current state of the KnowledgeStore.
     * @param cancelToken Allows asynchronous cooperative cancellation.
     * @return SemanticError::None on success, or an error reason.
     */
    SemanticError Orchestrate(const SemanticRequest& request, 
                              const std::vector<NetDiscovery::LogicalDevice>& availableDevices,
                              std::shared_ptr<std::atomic<bool>> cancelToken);

private:
    std::shared_ptr<NetDiscovery::ExecutionEngine> m_executionEngine;
    IntentCanonicalizer m_intentCanonicalizer;
    DeviceMatcher m_deviceMatcher;
    CapabilityFilter m_capabilityFilter;
    ParameterNormalizer m_parameterNormalizer;
    LogicalEntityResolver m_entityResolver;
    WorkflowPlanner m_workflowPlanner;
};

} // namespace semantic
