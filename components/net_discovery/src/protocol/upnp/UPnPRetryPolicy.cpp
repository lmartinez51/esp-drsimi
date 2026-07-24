/**
 * @file UPnPRetryPolicy.cpp
 * @brief Implementation of UPnPRetryPolicy (v5.0.0 Architecture Phase 11.1).
 */

#include "protocol/upnp/UPnPRetryPolicy.h"

#include <algorithm>

namespace NetDiscovery {
namespace Protocol {
namespace UPnP {

bool UPnPRetryPolicy::IsTransientError(int32_t statusCode) const {
    // 0 = connection timeout/socket error
    // 408 = Request Timeout
    // 429 = Too Many Requests
    // 500 = Internal Server Error (often transient in UPnP devices)
    // 502 = Bad Gateway
    // 503 = Service Unavailable
    // 504 = Gateway Timeout
    return (statusCode == 0 || statusCode == 408 || statusCode == 429 ||
            statusCode == 500 || statusCode == 502 || statusCode == 503 || statusCode == 504);
}

bool UPnPRetryPolicy::ShouldRetry(const UPnPResponse& response, uint32_t currentAttempt) const {
    if (currentAttempt >= m_maxRetries) return false;
    if (response.IsSuccess()) return false;
    return IsTransientError(response.statusCode);
}

uint32_t UPnPRetryPolicy::CalculateBackoffMs(uint32_t attempt) const {
    uint32_t shift = (attempt > 6u) ? 6u : attempt;
    return m_baseBackoffMs * (1u << shift);
}


} // namespace UPnP
} // namespace Protocol
} // namespace NetDiscovery
