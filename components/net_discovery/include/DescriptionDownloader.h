/**
 * @file DescriptionDownloader.h
 * @brief Orchestrates downloading and dispatching UPnP device descriptions.
 *
 * Pulls pending devices from the registry, uses HttpClient to download
 * their XML descriptions, wraps the XML in a Packet, and forwards it to
 * the AnalyzerDispatcher for parsing by XmlAnalyzer.
 */

#pragma once

#include "DeviceRegistry.h"
#include "AnalyzerDispatcher.h"

namespace NetDiscovery {

class DescriptionDownloader {
public:
    /**
     * @brief Constructor.
     * @param registry    Registry containing devices to process.
     * @param dispatcher  Dispatcher to route downloaded XML packets.
     */
    DescriptionDownloader(DeviceRegistry& registry, AnalyzerDispatcher& dispatcher);

    /**
     * @brief Fetch descriptions for all pending devices synchronously.
     * 
     * Iterates over registry.GetDevicesPendingDescription().
     * Designed to be easily adaptable to an async model in the future.
     */
    void ProcessPending();

private:
    DeviceRegistry& m_registry;
    AnalyzerDispatcher& m_dispatcher;
};

} // namespace NetDiscovery
