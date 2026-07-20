#include "TransportSelector.h"

namespace NetDiscovery {

TransportSelector::TransportSelector(const TransportRegistry& registry) 
    : registry(registry) {
}

std::shared_ptr<ICommunicationTransport> TransportSelector::SelectTransport(const ExecutionRoute& route) const {
    // In the future this might check endpoint protocols against transport capabilities,
    // but right now it relies directly on the strong typing.
    return registry.GetTransport(route.transport);
}

} // namespace NetDiscovery
