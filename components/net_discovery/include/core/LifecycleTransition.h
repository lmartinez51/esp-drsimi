/**
 * @file LifecycleTransition.h
 * @brief Represents a lifecycle state transition log or event descriptor.
 */

#pragma once

#include <string>
#include <cstdint>

namespace NetDiscovery {

/**
 * @brief Represents a lifecycle state transition log or event descriptor.
 */
struct LifecycleTransition {
    std::string entityId;
    std::string previousState;
    std::string newState;
    std::string reason;
    int64_t timestamp{0};
};

} // namespace NetDiscovery
