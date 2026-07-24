/**
 * @file MQTTResponse.h
 * @brief Immutable value object representing an MQTT operation response (v5.0.0 Architecture Phase 12).
 */

#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>
#include <utility>

namespace NetDiscovery {
namespace Protocol {
namespace MQTT {

/**
 * @brief Immutable response payload carrying status code, packet ID, return code, and diagnostics.
 *
 * Contains ZERO socket or parser logic. Pure value object.
 */
struct MQTTResponse {
    int32_t     statusCode{0};      ///< 0 = Success, >0 = MQTT error/reason code, <0 = transport failure
    std::string statusText;
    uint16_t    packetId{0};
    uint8_t     returnCode{0};      ///< MQTT CONNACK / SUBACK return code
    uint32_t    elapsedTimeMs{0};
    uint32_t    bytesTransmitted{0};
    uint32_t    bytesReceived{0};
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> metadata;

    MQTTResponse() = default;

    MQTTResponse(int32_t code,
                 std::string text,
                 uint16_t pid = 0,
                 uint8_t retCode = 0,
                 uint32_t elapsed = 0,
                 uint32_t txBytes = 0,
                 uint32_t rxBytes = 0)
        : statusCode(code)
        , statusText(std::move(text))
        , packetId(pid)
        , returnCode(retCode)
        , elapsedTimeMs(elapsed)
        , bytesTransmitted(txBytes)
        , bytesReceived(rxBytes) {}

    bool IsSuccess() const { return statusCode == 0; }
    bool IsTimeout() const { return statusCode == -101; }
    bool IsAuthError() const { return statusCode == 4 || statusCode == 5; }
};

} // namespace MQTT
} // namespace Protocol
} // namespace NetDiscovery
