/**
 * @file BoundExecutionRequest.h
 * @brief Strongly typed execution request context for pure execution dispatch (v5.0.0 Phase A).
 */

#pragma once

#include "LogicalDevice.h"
#include "IDeviceController.h"
#include "ActionDescriptor.h"
#include "transports/soap/SOAPExecutionContext.h"
#include <string>
#include <map>

#include "core/ExecutionPolicy.h"

namespace NetDiscovery {

/**
 * @brief Aggregate context object carrying resolved device reference, selected controller, action, policy, and parameters.
 */
struct BoundExecutionRequest {
    const LogicalDevice* targetDevice{nullptr};
    IDeviceController* selectedController{nullptr};
    ActionDescriptor action;
    std::map<std::string, std::string> parameters;
    ExecutionPolicy policy;
    ExecutionContext context; // Session / auth context
};

} // namespace NetDiscovery
