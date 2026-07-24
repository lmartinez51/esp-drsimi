/**
 * @file MQTTRequest.h
 * @brief Immutable value object representing an MQTT operation request (v5.0.0 Architecture Phase 12).
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
 * @brief Type of MQTT operation to execute.
 */
enum class MQTTOperationType {
    Publish,
    Subscribe,
    Unsubscribe,
    Disconnect
};

/**
 * @brief Converts MQTTOperationType to string.
 */
inline std::string ToString(MQTTOperationType op) {
    switch (op) {
        case MQTTOperationType::Publish:     return "Publish";
        case MQTTOperationType::Subscribe:   return "Subscribe";
        case MQTTOperationType::Unsubscribe: return "Unsubscribe";
        case MQTTOperationType::Disconnect:  return "Disconnect";
        default:                             return "Unknown";
    }
}

/**
 * @brief Immutable request payload carrying MQTT topic, payload, QoS, retain, and packet ID.
 *
 * Contains ZERO parsing or socket logic. Pure value object.
 */
struct MQTTRequest {
    MQTTOperationType operationType{MQTTOperationType::Publish};
    std::string       topic;
    std::string       payload;
    int               qos{0};
    bool              retain{false};
    uint16_t          packetId{0};
    uint32_t          timeoutMs{5000};
    std::unordered_map<std::string, std::string> metadata;

    MQTTRequest() = default;

    MQTTRequest(MQTTOperationType type,
                std::string top,
                std::string pay = "",
                int q = 0,
                bool ret = false,
                uint16_t pid = 0,
                uint32_t timeout = 5000)
        : operationType(type)
        , topic(std::move(top))
        , payload(std::move(pay))
        , qos(q)
        , retain(ret)
        , packetId(pid)
        , timeoutMs(timeout) {}

    bool IsValid() const { return operationType == MQTTOperationType::Disconnect || !topic.empty(); }
};

} // namespace MQTT
} // namespace Protocol
} // namespace NetDiscovery
