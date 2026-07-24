/**
 * @file UPnPErrorMapper.h
 * @brief Translates protocol-level UPnPResponse and UPnPParsedResponse into architectural ExecutionStepResult (v5.0.0 Architecture Phase 11.1).
 */

#pragma once

#include "protocol/upnp/UPnPResponse.h"
#include "protocol/upnp/UPnPResponseParser.h"
#include "runtime/ExecutionStepResult.h"

#include <string>

namespace NetDiscovery {
namespace Protocol {
namespace UPnP {

/**
 * @brief Error translation engine mapping UPnPResponse failures to architecture-level ExecutionStepResult.
 *
 * Guaranteed Invariant: SOAP Fault codes, HTTP status codes, and XML parse errors
 * terminate strictly inside the adapter and are NEVER exposed as raw exceptions or raw codes outside the adapter.
 */
class UPnPErrorMapper {
public:
    UPnPErrorMapper() = default;

    /**
     * @brief Maps UPnPResponse and parsed SOAP outcome into an ExecutionStepResult.
     */
    Runtime::ExecutionStepResult MapToStepResult(
        const std::string&         stepId,
        const std::string&         adapterId,
        const UPnPResponse&        response,
        const UPnPParsedResponse&  parsed) const;
};

} // namespace UPnP
} // namespace Protocol
} // namespace NetDiscovery
