/**
 * @file MQTTResponseMapper.cpp
 * @brief Implementation of MQTTResponseMapper (v5.0.0 Architecture Phase 12).
 */

#include "protocol/mqtt/MQTTResponseMapper.h"

namespace NetDiscovery {
namespace Protocol {
namespace MQTT {

Runtime::ExecutionStepResult MQTTResponseMapper::MapToStepResult(
        const std::string&   stepId,
        const std::string&   adapterId,
        const MQTTResponse&  response) const {

    // 1. Success path
    if (response.IsSuccess()) {
        return Runtime::ExecutionStepResult(
            stepId,
            Execution::StepStatus::Success,
            adapterId,
            response.elapsedTimeMs,
            response.elapsedTimeMs,
            0,
            "",
            false, false,
            {"mqttStatus=OK", "packetId=" + std::to_string(response.packetId)},
            {{"packetId", std::to_string(response.packetId)},
             {"bytesTransmitted", std::to_string(response.bytesTransmitted)},
             {"bytesReceived", std::to_string(response.bytesReceived)}},
            {{"protocol", "MQTT"}, {"transport", "TCP/TLS"}});
    }

    // 2. Timeout
    if (response.IsTimeout()) {
        return Runtime::ExecutionStepResult(
            stepId,
            Execution::StepStatus::Timeout,
            adapterId,
            response.elapsedTimeMs,
            response.elapsedTimeMs,
            -101,
            "MQTT Timeout waiting for ACK (" + response.statusText + ")",
            true, false,
            {"timeoutMs=" + std::to_string(response.elapsedTimeMs)},
            {},
            {{"protocol", "MQTT"}, {"errorType", "Timeout"}});
    }

    // 3. Authentication Failure
    if (response.IsAuthError()) {
        return Runtime::ExecutionStepResult(
            stepId,
            Execution::StepStatus::Failure,
            adapterId,
            response.elapsedTimeMs,
            response.elapsedTimeMs,
            response.statusCode,
            "MQTT Authentication / Authorization Failed (code=" + std::to_string(response.returnCode) + ")",
            false, false,
            {"returnCode=" + std::to_string(response.returnCode)},
            {},
            {{"protocol", "MQTT"}, {"errorType", "AuthenticationFailure"}});
    }

    // 4. Protocol / NACK Failure
    bool retrySuggested = (response.statusCode == 1 || response.statusCode == 2); // Unacceptable protocol version or Server unavailable

    return Runtime::ExecutionStepResult(
        stepId,
        Execution::StepStatus::Failure,
        adapterId,
        response.elapsedTimeMs,
        response.elapsedTimeMs,
        response.statusCode,
        "MQTT Protocol Failure: " + response.statusText,
        retrySuggested, false,
        {"statusCode=" + std::to_string(response.statusCode)},
        {},
        {{"protocol", "MQTT"}, {"errorType", "ProtocolFailure"}});
}

} // namespace MQTT
} // namespace Protocol
} // namespace NetDiscovery
