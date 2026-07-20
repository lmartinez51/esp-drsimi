/**
 * @file ExecutionEngine.h
 * @brief Internal orchestrator for executing commands on a device.
 */

#pragma once

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

    ExecutionResult Execute(const ExecutionRequest& request);

private:
    TransportSelector transportSelector;
    const ControllerRegistry& controllerRegistry;
    std::shared_ptr<AuthenticationManager> authManager;
};

} // namespace NetDiscovery
