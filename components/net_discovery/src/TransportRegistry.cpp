#include "TransportRegistry.h"

namespace NetDiscovery {

void TransportRegistry::RegisterTransport(std::shared_ptr<ICommunicationTransport> transport) {
    if (transport) {
        transports.push_back(transport);
    }
}

std::shared_ptr<ICommunicationTransport> TransportRegistry::GetTransport(TransportFamily family) const {
    for (const auto& transport : transports) {
        if (transport->GetFamily() == family) {
            return transport;
        }
    }
    // Fallback: If we couldn't find a specific transport but we have a dummy one, use it for Phase 5 architecture testing.
    for (const auto& transport : transports) {
        if (transport->GetFamily() == TransportFamily::Unknown) {
            return transport;
        }
    }
    return nullptr;
}

const std::vector<std::shared_ptr<ICommunicationTransport>>& TransportRegistry::GetAllTransports() const {
    return transports;
}

} // namespace NetDiscovery
