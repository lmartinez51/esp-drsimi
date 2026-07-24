/**
 * @file ExecutionEngine.h
 * @brief Internal orchestrator for executing commands on a device.
 */

#pragma once

#include "core/BoundExecutionRequest.h"
#include "core/ExecutionRequest.h"
#include "core/ExecutionResult.h"
#include "TransportSelector.h"
#include "ControllerRegistry.h"
#include "core/AuthenticationManager.h"
#include <memory>

namespace NetDiscovery {

class ExecutionEngine {
public:
    ExecutionEngine(const TransportRegistry& transportRegistry, 
                    const ControllerRegistry& controllerRegistry,
                    std::shared_ptr<AuthenticationManager> authManager = nullptr);

    /**
     * @brief Single pure execution entry point.
     */
    ExecutionResult Execute(const BoundExecutionRequest& request);

private:
    TransportSelector transportSelector;
    const ControllerRegistry& controllerRegistry;
    std::shared_ptr<AuthenticationManager> authManager;
};

} // namespace NetDiscovery
