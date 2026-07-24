/**
 * @file WaitResult.h
 * @brief Enum for IEventSignal wait outcomes (v6.0 Phase D).
 */

#pragma once

#include <cstdint>

namespace NetDiscovery {
namespace Plan {

enum class WaitResult : uint8_t {
    Signalled,
    TimedOut,
    Cancelled
};

} // namespace Plan
} // namespace NetDiscovery
