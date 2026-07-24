/**
 * @file HTTPRetryPolicy.cpp
 * @brief Implementation of HTTPRetryPolicy (v5.0.0 Architecture Phase 15).
 */

#include "protocol/http/HTTPRetryPolicy.h"

namespace NetDiscovery {
namespace Protocol {
namespace HTTP {

bool HTTPRetryPolicy::IsTransientError(int32_t statusCode) const {
    // -101 = Timeout
    // 502  = Bad Gateway
    // 503  = Service Unavailable
    // 504  = Gateway Timeout
    return (statusCode == -101 || statusCode == 502 || statusCode == 503 || statusCode == 504);
}

bool HTTPRetryPolicy::ShouldRetry(const HTTPResponse& response, uint32_t currentAttempt) const {
    if (currentAttempt >= m_maxRetries) return false;
    if (response.IsSuccess()) return false;
    return IsTransientError(response.statusCode);
}

uint32_t HTTPRetryPolicy::CalculateBackoffMs(uint32_t attempt) const {
    uint32_t shift = (attempt > 6u) ? 6u : attempt;
    return m_baseBackoffMs * (1u << shift);
}

} // namespace HTTP
} // namespace Protocol
} // namespace NetDiscovery
