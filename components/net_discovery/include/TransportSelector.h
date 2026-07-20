/**
 * @file TransportSelector.h
 * @brief Selects the most appropriate transport based on the ExecutionRoute.
 */

#pragma once

#include "TransportRegistry.h"
#include "core/ExecutionRoute.h"
#include <memory>

namespace NetDiscovery {

class TransportSelector {
public:
    explicit TransportSelector(const TransportRegistry& registry);

    std::shared_ptr<ICommunicationTransport> SelectTransport(const ExecutionRoute& route) const;

private:
    const TransportRegistry& registry;
};

} // namespace NetDiscovery
