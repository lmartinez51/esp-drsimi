/**
 * @file DefaultRuntimeClock.h
 * @brief Default implementation of IRuntimeClock using esp_timer (v6.0 Phase C.5).
 */

#pragma once

#include "plan/IRuntimeClock.h"
#include "esp_timer.h"

namespace NetDiscovery {
namespace Plan {

class DefaultRuntimeClock : public IRuntimeClock {
public:
    uint64_t GetCurrentTimeMs() const override {
        return static_cast<uint64_t>(esp_timer_get_time() / 1000);
    }
};

} // namespace Plan
} // namespace NetDiscovery
