/**
 * @file ExecutionRequest.h
 * @brief Represents a request to execute an action on a LogicalDevice.
 */

#pragma once

#include "ActionDescriptor.h"
#include "ExecutionContext.h"
#include <string>
#include <map>

namespace NetDiscovery {

// Forward declaration since LogicalDevice isn't needed fully here
struct LogicalDevice;

/**
 * @brief Contains the payload to be executed against a device.
 */
struct ExecutionRequest {
    const LogicalDevice& device;
    ActionDescriptor action;
    std::map<std::string, std::string> parameters;
    ExecutionContext context;
    int timeoutMs{5000};
    int executionFlags{0}; // For future use
};

} // namespace NetDiscovery
