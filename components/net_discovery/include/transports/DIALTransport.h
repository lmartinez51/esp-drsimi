/**
 * @file DIALTransport.h
 * @brief Communication transport for the DIAL protocol.
 */

#pragma once

#include "../ICommunicationTransport.h"
#include "../HttpClient.h"

namespace NetDiscovery {

class DIALTransport : public ICommunicationTransport {
public:
    TransportFamily GetFamily() const override {
        return TransportFamily::DIAL;
    }

    ExecutionResult Execute(const ExecutionRequest& request, 
                            const ExecutionRoute& route) override;
};

} // namespace NetDiscovery
