/**
 * @file MQTTSessionContext.h
 * @brief Mutable runtime session state for MQTT communications (v5.0.0 Architecture Phase 12).
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <utility>

namespace NetDiscovery {
namespace Protocol {
namespace MQTT {

/**
 * @brief Mutable runtime state for an MQTT protocol adapter session.
 *
 * Owned per MQTT client connection. Strictly separated from immutable MQTTBrokerContext.
 */
struct MQTTSessionContext {
    std::string clientId;                       ///< Unique MQTT client identifier
    bool        sessionPresent{false};          ///< Clean session / session present flag
    std::string keepAliveState{"Active"};       ///< Keep-alive timer status
    uint32_t    reconnectCounter{0};            ///< Total reconnect attempts
    std::unordered_map<std::string, int> subscriptionState; ///< Active topic subscriptions -> QoS
    std::vector<uint16_t> pendingPacketIds;     ///< In-flight QoS 1/2 packet IDs
    std::string authenticationState{"Authenticated"};
    uint64_t    lastActivityTimestampMs{0};     ///< Last PING / PUBLISH activity
    uint64_t    stateVersion{0};                ///< Monotonic version incrementer
    std::unordered_map<std::string, std::string> sessionVariables;

    MQTTSessionContext() = default;
    explicit MQTTSessionContext(std::string id)
        : clientId(std::move(id)) {}

    void BumpVersion() { ++stateVersion; }
};

} // namespace MQTT
} // namespace Protocol
} // namespace NetDiscovery
