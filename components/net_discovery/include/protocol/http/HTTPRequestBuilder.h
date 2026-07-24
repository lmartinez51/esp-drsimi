/**
 * @file HTTPRequestBuilder.h
 * @brief Pure translation component converting ExecutionStep into HTTPRequest (v5.0.0 Architecture Phase 15).
 */

#pragma once

#include "protocol/http/HTTPRequest.h"
#include "execution/ExecutionStep.h"

#include <string>

namespace NetDiscovery {
namespace Protocol {
namespace HTTP {

/**
 * @brief Pure translator constructing HTTPRequest instances from ExecutionStep descriptors.
 *
 * Performs ZERO network operations, opens ZERO sockets, and reads ZERO external state.
 */
class HTTPRequestBuilder {
public:
    HTTPRequestBuilder() = default;

    /**
     * @brief Translates an ExecutionStep into an immutable HTTPRequest.
     */
    HTTPRequest BuildRequest(const Execution::ExecutionStep& step,
                             const std::string& baseUrl = "") const;
};

} // namespace HTTP
} // namespace Protocol
} // namespace NetDiscovery
