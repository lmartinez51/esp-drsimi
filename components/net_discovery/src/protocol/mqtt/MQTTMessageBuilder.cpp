/**
 * @file MQTTMessageBuilder.cpp
 * @brief Implementation of MQTTMessageBuilder (v5.0.0 Architecture Phase 12).
 */

#include "protocol/mqtt/MQTTMessageBuilder.h"

#include <cstdlib>

namespace NetDiscovery {
namespace Protocol {
namespace MQTT {

MQTTRequest MQTTMessageBuilder::BuildRequest(const Execution::ExecutionStep& step) const {
    MQTTOperationType opType = MQTTOperationType::Publish;
    const std::string opId   = step.GetOperationId();

    if (opId == "Subscribe") {
        opType = MQTTOperationType::Subscribe;
    } else if (opId == "Unsubscribe") {
        opType = MQTTOperationType::Unsubscribe;
    } else if (opId == "Disconnect") {
        opType = MQTTOperationType::Disconnect;
    }

    std::string topic;
    std::string payload;
    int qos = 0;
    bool retain = false;
    uint16_t packetId = 0;

    const auto& params = step.GetParameterValues();

    auto tIt = params.find("topic");
    if (tIt != params.end()) topic = tIt->second;

    auto pIt = params.find("payload");
    if (pIt != params.end()) payload = pIt->second;

    auto qIt = params.find("qos");
    if (qIt != params.end()) qos = std::atoi(qIt->second.c_str());

    auto rIt = params.find("retain");
    if (rIt != params.end()) retain = (rIt->second == "true" || rIt->second == "1");

    auto idIt = params.find("packetId");
    if (idIt != params.end()) packetId = static_cast<uint16_t>(std::atoi(idIt->second.c_str()));

    uint32_t timeoutMs = step.GetTimeoutMs() > 0 ? step.GetTimeoutMs() : 5000;

    return MQTTRequest(opType, topic, payload, qos, retain, packetId, timeoutMs);
}

} // namespace MQTT
} // namespace Protocol
} // namespace NetDiscovery
