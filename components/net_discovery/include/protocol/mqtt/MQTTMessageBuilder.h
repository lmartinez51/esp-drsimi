/**
 * @file MQTTMessageBuilder.h
 * @brief Pure translation component converting ExecutionStep into MQTTRequest (v5.0.0 Architecture Phase 12).
 */

#pragma once

#include "protocol/mqtt/MQTTRequest.h"
#include "execution/ExecutionStep.h"

#include <string>

namespace NetDiscovery {
namespace Protocol {
namespace MQTT {

/**
 * @brief Pure translator constructing MQTTRequest instances from ExecutionStep descriptors.
 *
 * Performs ZERO network operations, opens ZERO sockets, and reads ZERO external state.
 */
class MQTTMessageBuilder {
public:
    MQTTMessageBuilder() = default;

    /**
     * @brief Translates an ExecutionStep into an immutable MQTTRequest.
     *
     * Examines step.GetOperationId() ("Publish", "Subscribe", "Unsubscribe", "Disconnect")
     * and step.GetParameterValues() ("topic", "payload", "qos", "retain", "packetId").
     */
    MQTTRequest BuildRequest(const Execution::ExecutionStep& step) const;
};

} // namespace MQTT
} // namespace Protocol
} // namespace NetDiscovery
