/**
 * @file HTTPRetryPolicy.h
 * @brief Policy component evaluating retry decisions for HTTP operations (v5.0.0 Architecture Phase 15).
 */

#pragma once

#include "protocol/http/HTTPResponse.h"

#include <cstdint>

namespace NetDiscovery {
namespace Protocol {
namespace HTTP {

/**
 * @brief Evaluates whether an HTTP failure should be retried and calculates backoff intervals.
 *
 * Isolated policy class. Consumed by HTTPAdapter to decide retry attempts.
 */
class HTTPRetryPolicy {
public:
    explicit HTTPRetryPolicy(uint32_t maxRetries = 3, uint32_t baseBackoffMs = 200)
        : m_maxRetries(maxRetries)
        , m_baseBackoffMs(baseBackoffMs) {}

    /**
     * @brief Determines whether a failed response should trigger a retry attempt.
     */
    bool ShouldRetry(const HTTPResponse& response, uint32_t currentAttempt) const;

    /**
     * @brief Calculates exponential backoff duration in milliseconds.
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

} // namespace HTTP
} // namespace Protocol
} // namespace NetDiscovery
