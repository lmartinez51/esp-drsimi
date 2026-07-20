/**
 * @file DeviceExecutor.h
 * @brief Public façade for executing commands on devices.
 */

#pragma once

#include "ExecutionEngine.h"
#include "core/AuthenticationManager.h"
#include <memory>

namespace NetDiscovery {

/**
 * @brief The stable public API for executing actions.
 * 
 * This class wraps the internal ExecutionEngine and serves as the single
 * entry point for future clients (like ESP-Claw / Lua scripts) to execute
 * high-level actions on physical devices without needing to understand
 * protocol specifics or execution routing.
 */
class DeviceExecutor {
public:
    DeviceExecutor(const TransportRegistry& transportRegistry, 
                   const ControllerRegistry& controllerRegistry,
                   std::shared_ptr<AuthenticationManager> authManager = nullptr);

    ExecutionResult Execute(const ExecutionRequest& request);

private:
    ExecutionEngine engine;
};

} // namespace NetDiscovery
