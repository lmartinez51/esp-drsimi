/**
 * @file MQTTRetryPolicy.cpp
 * @brief Implementation of MQTTRetryPolicy (v5.0.0 Architecture Phase 12).
 */

#include "protocol/mqtt/MQTTRetryPolicy.h"

namespace NetDiscovery {
namespace Protocol {
namespace MQTT {

bool MQTTRetryPolicy::IsTransientError(int32_t statusCode) const {
    // -101 = Timeout
    // 2    = Identifier Rejected (transient on broker restart)
    // 3    = Server Unavailable
    return (statusCode == -101 || statusCode == 2 || statusCode == 3);
}

bool MQTTRetryPolicy::ShouldRetry(const MQTTResponse& response, uint32_t currentAttempt) const {
    if (currentAttempt >= m_maxRetries) return false;
    if (response.IsSuccess()) return false;
    return IsTransientError(response.statusCode);
}

uint32_t MQTTRetryPolicy::CalculateBackoffMs(uint32_t attempt) const {
    uint32_t shift = (attempt > 6u) ? 6u : attempt;
    return m_baseBackoffMs * (1u << shift);
}

} // namespace MQTT
} // namespace Protocol
} // namespace NetDiscovery
