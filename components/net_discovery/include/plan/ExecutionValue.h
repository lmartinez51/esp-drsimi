/**
 * @file ExecutionValue.h
 * @brief Strongly typed variant value system for ExecutionPlanContext blackboard (v6.0 Phase C).
 */

#pragma once

#include "core/ExecutionResult.h"
#include "core/LogicalDevice.h"
#include <variant>
#include <string>
#include <cstdint>

namespace NetDiscovery {
namespace Plan {

using ExecutionValue = std::variant<
    std::monostate,
    bool,
    int64_t,
    double,
    std::string,
    NetDiscovery::ExecutionResult,
    NetDiscovery::LogicalDevice
>;

} // namespace Plan
} // namespace NetDiscovery
