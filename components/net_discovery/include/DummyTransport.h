/**
 * @file DummyTransport.h
 * @brief Placeholder transport for validating execution architecture.
 */

#pragma once

#include "ICommunicationTransport.h"

namespace NetDiscovery {

class DummyTransport : public ICommunicationTransport {
public:
    TransportFamily GetFamily() const override;

    ExecutionResult Execute(
        const ExecutionRequest& request, 
        const ExecutionRoute& route) override;
};

} // namespace NetDiscovery
