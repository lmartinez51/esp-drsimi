/**
 * @file ExecutionInfrastructure.h
 * @brief ExecutionInfrastructure pipeline stage orchestrating reachability, retries, and auth (v5.1.0 Phase B).
 */

#pragma once

#include "ExecutionEngine.h"
#include "core/BoundExecutionRequest.h"
#include "infrastructure/ReachabilityVerifierFactory.h"
#include <memory>

namespace NetDiscovery {

/**
 * @brief Pipeline stage sitting between SemanticOrchestrator/Controller and ExecutionEngine.
 *
 * Orchestrates infrastructure policies (Reachability, Retries, Timeouts, Auth)
 * before delegating pure transport dispatch to ExecutionEngine.
 */
class ExecutionInfrastructure {
public:
    ExecutionInfrastructure(std::shared_ptr<ExecutionEngine> executionEngine);

    ExecutionResult ExecuteWithPolicy(const BoundExecutionRequest& request);

private:
    std::shared_ptr<ExecutionEngine> m_executionEngine;
};

} // namespace NetDiscovery
