/**
 * @file MQTTRetryPolicy.h
 * @brief Policy component evaluating retry decisions for MQTT operations (v5.0.0 Architecture Phase 12).
 */

#pragma once

#include "protocol/mqtt/MQTTResponse.h"

#include <cstdint>

namespace NetDiscovery {
namespace Protocol {
namespace MQTT {

/**
 * @brief Evaluates whether an MQTT failure should be retried and calculates backoff intervals.
 *
 * Isolated policy class. Consumed by MQTTAdapter to decide retry attempts.
 */
class MQTTRetryPolicy {
public:
    explicit MQTTRetryPolicy(uint32_t maxRetries = 3, uint32_t baseBackoffMs = 250)
        : m_maxRetries(maxRetries)
        , m_baseBackoffMs(baseBackoffMs) {}

    /**
     * @brief Determines whether a failed response should trigger a retry attempt.
     */
    bool ShouldRetry(const MQTTResponse& response, uint32_t currentAttempt) const;

    /**
     * @brief Calculates exponential backoff duration in milliseconds.
     */
    uint32_t CalculateBackoffMs(uint32_t attempt) const;

    /**
     * @brief Returns true if an MQTT error code represents a transient failure.
     */
    bool IsTransientError(int32_t statusCode) const;

private:
    uint32_t m_maxRetries{3};
    uint32_t m_baseBackoffMs{250};
};

} // namespace MQTT
} // namespace Protocol
} // namespace NetDiscovery
