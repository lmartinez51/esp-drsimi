/**
 * @file WakeOnLANTransport.h
 * @brief Sends Magic Packets to wake up sleeping devices.
 */

#pragma once

#include "ICommunicationTransport.h"

namespace NetDiscovery {

class WakeOnLANTransport : public ICommunicationTransport {
public:
    TransportFamily GetFamily() const override {
        return TransportFamily::WakeOnLAN;
    }

    ExecutionResult Execute(const ExecutionRequest& request, const ExecutionRoute& route) override;
};

} // namespace NetDiscovery
