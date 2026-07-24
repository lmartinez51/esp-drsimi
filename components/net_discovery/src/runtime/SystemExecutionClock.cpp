/**
 * @file SystemExecutionClock.cpp
 * @brief Production IExecutionClock implementation backed by esp_timer (v5.0.0 Architecture Phase 9.2).
 */

#include "runtime/ExecutionClock.h"
#include "esp_timer.h"

namespace NetDiscovery {
namespace Runtime {

uint64_t SystemExecutionClock::NowMs() const {
    return static_cast<uint64_t>(esp_timer_get_time() / 1000);
}

uint64_t SystemExecutionClock::ElapsedMs(uint64_t startMs) const {
    uint64_t now = NowMs();
    return (now >= startMs) ? (now - startMs) : 0;
}

} // namespace Runtime
} // namespace NetDiscovery
