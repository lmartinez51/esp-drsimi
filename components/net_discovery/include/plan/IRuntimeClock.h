/**
 * @file IRuntimeClock.h
 * @brief Abstract clock interface for timestamping and duration measurement (v6.0 Phase C.5).
 */

#pragma once

#include <cstdint>

namespace NetDiscovery {
namespace Plan {

class IRuntimeClock {
public:
    virtual ~IRuntimeClock() = default;

    /**
     * @brief Returns current time in milliseconds.
     */
    virtual uint64_t GetCurrentTimeMs() const = 0;
};

} // namespace Plan
} // namespace NetDiscovery
