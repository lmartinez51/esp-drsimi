/**
 * @file UPnPRetryPolicy.h
 * @brief Policy component evaluating retry decisions and backoff calculations (v5.0.0 Architecture Phase 11.1).
 */

#pragma once

#include "protocol/upnp/UPnPResponse.h"

#include <cstdint>

namespace NetDiscovery {
namespace Protocol {
namespace UPnP {

/**
 * @brief Evaluates whether an HTTP/SOAP failure should be retried and calculates backoff intervals.
 *
 * Isolated policy class. Consumed by UPnPAdapter to decide retry attempts.
 */
class UPnPRetryPolicy {
public:
    explicit UPnPRetryPolicy(uint32_t maxRetries = 3, uint32_t baseBackoffMs = 200)
        : m_maxRetries(maxRetries)
        , m_baseBackoffMs(baseBackoffMs) {}

    /**
     * @brief Determines whether a failed response should trigger a retry attempt.
     */
    bool ShouldRetry(const UPnPResponse& response, uint32_t currentAttempt) const;

    /**
     * @brief Calculates backoff wait duration in milliseconds using exponential backoff.
     */
    uint32_t CalculateBackoffMs(uint32_t attempt) const;

    /**
     * @brief Returns true if an HTTP status code represents a transient error.
     */
    bool IsTransientError(int32_t statusCode) const;

private:
    uint32_t m_maxRetries{3};
    uint32_t m_baseBackoffMs{200};
};

} // namespace UPnP
} // namespace Protocol
} // namespace NetDiscovery
