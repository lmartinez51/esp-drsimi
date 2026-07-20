/**
 * @file TransportRegistry.h
 * @brief Manages the available execution transports.
 */

#pragma once

#include "ICommunicationTransport.h"
#include <memory>
#include <vector>

namespace NetDiscovery {

class TransportRegistry {
public:
    void RegisterTransport(std::shared_ptr<ICommunicationTransport> transport);
    
    std::shared_ptr<ICommunicationTransport> GetTransport(TransportFamily family) const;

    const std::vector<std::shared_ptr<ICommunicationTransport>>& GetAllTransports() const;

private:
    std::vector<std::shared_ptr<ICommunicationTransport>> transports;
};

} // namespace NetDiscovery
