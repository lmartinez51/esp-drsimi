/**
 * @file MQTTBrokerContext.h
 * @brief Immutable metadata for an MQTT broker (v5.0.0 Architecture Phase 12).
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <utility>

namespace NetDiscovery {
namespace Protocol {
namespace MQTT {

/**
 * @brief Immutable metadata container for an MQTT broker target.
 *
 * Contains ZERO mutable runtime session variables.
 */
struct MQTTBrokerContext {
    std::string brokerUri;                  ///< e.g., "mqtt://broker.hivemq.com:1883"
    std::string hostname;                   ///< e.g., "broker.hivemq.com"
    uint16_t    port{1883};                 ///< Default MQTT port
    bool        useTls{false};              ///< TLS encryption flag
    std::string protocolVersion{"3.1.1"};   ///< "3.1.1" or "5.0"
    std::vector<std::string> capabilities;  ///< e.g., "qos0", "qos1", "qos2", "retain"
    bool        authenticationRequired{false};
    std::unordered_map<std::string, std::string> metadata;

    MQTTBrokerContext() = default;

    MQTTBrokerContext(std::string uri,
                      std::string host,
                      uint16_t p = 1883,
                      bool tls = false,
                      std::string ver = "3.1.1",
                      std::vector<std::string> caps = {"qos0", "qos1", "retain"})
        : brokerUri(std::move(uri))
        , hostname(std::move(host))
        , port(p)
        , useTls(tls)
        , protocolVersion(std::move(ver))
        , capabilities(std::move(caps)) {}

    bool IsValid() const { return !brokerUri.empty() || !hostname.empty(); }
};

} // namespace MQTT
} // namespace Protocol
} // namespace NetDiscovery
