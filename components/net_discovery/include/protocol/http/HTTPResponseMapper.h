/**
 * @file HTTPResponseMapper.h
 * @brief Translates protocol-level HTTPResponse into architectural ExecutionStepResult (v5.0.0 Architecture Phase 15).
 */

#pragma once

#include "protocol/http/HTTPResponse.h"
#include "protocol/http/HTTPResponseParser.h"
#include "runtime/ExecutionStepResult.h"

#include <string>

namespace NetDiscovery {
namespace Protocol {
namespace HTTP {

/**
 * @brief Error and response translation engine mapping HTTP status codes to ExecutionStepResult.
 *
 * Guaranteed Invariant: Raw HTTP status codes terminate strictly inside the adapter
 * and are NEVER exposed as raw codes outside the adapter.
 */
class HTTPResponseMapper {
public:
    HTTPResponseMapper() = default;

    /**
     * @brief Maps HTTPResponse and HTTPParsedResponse into a fully populated ExecutionStepResult.
     */
    Runtime::ExecutionStepResult MapToStepResult(
        const std::string&         stepId,
        const std::string&         adapterId,
        const HTTPResponse&        response,
        const HTTPParsedResponse&  parsed) const;
};

} // namespace HTTP
} // namespace Protocol
} // namespace NetDiscovery
