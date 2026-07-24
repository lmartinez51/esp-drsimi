/**
 * @file StepOutputDescriptor.h
 * @brief Output descriptor declaration for IExecutionStep (v6.0 Phase D).
 */

#pragma once

#include <string>

namespace NetDiscovery {
namespace Plan {

struct StepOutputDescriptor {
    std::string name;
    std::string targetBlackboardKey;

    StepOutputDescriptor(std::string n, std::string key)
        : name(std::move(n)), targetBlackboardKey(std::move(key)) {}
};

} // namespace Plan
} // namespace NetDiscovery
