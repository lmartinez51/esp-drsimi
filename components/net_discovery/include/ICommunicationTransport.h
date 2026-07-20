/**
 * @file ICommunicationTransport.h
 * @brief Base interface for all network execution transports.
 */

#pragma once

#include "core/ExecutionRequest.h"
#include "core/ExecutionResult.h"
#include "core/ExecutionRoute.h"
#include "core/TransportFamily.h"

namespace NetDiscovery {

/**
 * @brief Base interface for all network execution transports.
 * 
 * IMPORTANT: Execute() must be synchronous and blocking, utilizing internal 
 * timeouts provided in the ExecutionRequest. Do not use callbacks or promises.
 */
class ICommunicationTransport {
public:
    virtual ~ICommunicationTransport() = default;

    virtual TransportFamily GetFamily() const = 0;

    virtual ExecutionResult Execute(
        const ExecutionRequest& request, 
        const ExecutionRoute& route) = 0;
};

} // namespace NetDiscovery
