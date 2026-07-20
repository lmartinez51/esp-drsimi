/**
 * @file SamsungWebSocketStrategy.h
 * @brief Constructs JSON payloads for Samsung WebSocket API.
 */

#pragma once

#include <string>
#include "../../core/IExecutionStrategy.h"

namespace NetDiscovery {
namespace Strategy {

/**
 * @brief Generates payload strings for the Samsung Remote Control WebSocket protocol.
 */
class SamsungWebSocketStrategy : public IExecutionStrategy {
public:
    /**
     * @brief Transforms the logical request into a physical transport-ready request.
     *        Constructs the URL, query parameters, and JSON payload.
     */
    void BuildRequest(ExecutionRequest& request, ExecutionRoute& route) const override;

    /**
     * @brief Processes the response payload from the WebSocket execution.
     * Extracts tokens and handles vendor-specific error codes like ms.channel.unauthorized.
     */
    void ProcessResponse(ExecutionResult& result, const ExecutionContext& context) const override;

private:
    std::string BuildKeyPayload(const std::string& keyName) const;
    std::string BuildWebSocketUrl(const std::string& token) const;
};

} // namespace Strategy
} // namespace NetDiscovery
