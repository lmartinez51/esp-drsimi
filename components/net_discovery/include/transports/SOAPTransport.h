#pragma once

#include "../ICommunicationTransport.h"

namespace NetDiscovery {

/**
 * @brief Communication transport for the SOAP protocol.
 * Completely decoupled from specific service implementations.
 * Consumes a generated SOAPRequest directly from the ExecutionRoute.
 */
class SOAPTransport : public ICommunicationTransport {
public:
    TransportFamily GetFamily() const override {
        return TransportFamily::SOAP;
    }

    /**
     * @brief Executes a SOAP Request via HTTP.
     * @param request The original semantic execution request (used for tracking).
     * @param route Must contain a valid ISOAPRequestBuilder in the transportContext.
     */
    ExecutionResult Execute(const ExecutionRequest& request, const ExecutionRoute& route) override;
};

} // namespace NetDiscovery
