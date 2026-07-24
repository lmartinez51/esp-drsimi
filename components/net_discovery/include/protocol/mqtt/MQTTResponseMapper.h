/**
 * @file MQTTResponseMapper.h
 * @brief Translates protocol-level MQTTResponse into architectural ExecutionStepResult (v5.0.0 Architecture Phase 12).
 */

#pragma once

#include "protocol/mqtt/MQTTResponse.h"
#include "runtime/ExecutionStepResult.h"

#include <string>

namespace NetDiscovery {
namespace Protocol {
namespace MQTT {

/**
 * @brief Error and response translation engine mapping MQTT acknowledgements to ExecutionStepResult.
 *
 * Guaranteed Invariant: MQTT return codes, CONNACK/SUBACK status codes, and packet errors
 * terminate strictly inside the adapter and are NEVER exposed as raw codes outside the adapter.
 */
class MQTTResponseMapper {
public:
    MQTTResponseMapper() = default;

    /**
     * @brief Maps MQTTResponse into a fully populated ExecutionStepResult.
     */
    Runtime::ExecutionStepResult MapToStepResult(
        const std::string&   stepId,
        const std::string&   adapterId,
        const MQTTResponse&  response) const;
};

} // namespace MQTT
} // namespace Protocol
} // namespace NetDiscovery
