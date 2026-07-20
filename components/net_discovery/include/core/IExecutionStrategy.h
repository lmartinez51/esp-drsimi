/**
 * @file IExecutionStrategy.h
 * @brief Abstract interface for vendor-specific execution strategies.
 */

#pragma once

#include "ExecutionRequest.h"
#include "ExecutionResult.h"

namespace NetDiscovery {

struct ExecutionRoute; // Forward declaration

/**
 * @brief Represents a vendor-specific strategy for building protocol requests 
 *        and processing protocol responses.
 */
class IExecutionStrategy {
public:
    virtual ~IExecutionStrategy() = default;

    /**
     * @brief Transforms the logical request into a physical transport-ready request.
     *        Can modify the route metadata (e.g. constructing paths, generating payloads).
     */
    virtual void BuildRequest(ExecutionRequest& request, ExecutionRoute& route) const = 0;

    /**
     * @brief Interprets the raw transport response and updates the execution result 
     *        or context (e.g. extracting tokens).
     */
    virtual void ProcessResponse(ExecutionResult& result, const ExecutionContext& context) const = 0;
};

} // namespace NetDiscovery
