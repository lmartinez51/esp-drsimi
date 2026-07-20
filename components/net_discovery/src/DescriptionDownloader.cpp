/**
 * @file DescriptionDownloader.cpp
 * @brief DescriptionDownloader implementation.
 */

#include "../include/DescriptionDownloader.h"
#include "../include/HttpClient.h"
#include "../include/core/Packet.h"
#include <map>
#include <iostream>
#include "esp_log.h"

static const char* TAG = "NetDiscovery";

extern bool g_verbose;

namespace NetDiscovery {

DescriptionDownloader::DescriptionDownloader(DeviceRegistry& registry, AnalyzerDispatcher& dispatcher)
    : m_registry(registry), m_dispatcher(dispatcher)
{
}

void DescriptionDownloader::ProcessPending()
{
    auto pending = m_registry.GetEvidencePendingDescription();
    if (pending.empty()) return;

    HttpClient client;

    for (const auto& dev : pending) {
        if (!dev.protocolEvidence.upnp.has_value() || dev.protocolEvidence.upnp->locationUrl.empty()) continue;
        std::string locationUrl = dev.protocolEvidence.upnp->locationUrl;

        auto resOpt = client.Get(locationUrl);
        if (!resOpt.has_value()) {
            ESP_LOGE(TAG, "[DescriptionDownloader] Failed to fetch %s for UUID %s", locationUrl.c_str(), dev.uuid.c_str());
            continue;
        }
        
        HttpResponse res = resOpt.value();
        
        // Create a packet representing the fetched XML
        Packet packet;
        packet.protocol = ProtocolType::XML;
        packet.rawPayload = res.body;
        packet.metadata["LOCATION"] = locationUrl;
        packet.metadata["UUID"] = dev.uuid;
        packet.source.address = dev.ip;

        // Cache Application-URL for DIAL transport if present
        auto it = res.headers.find("Application-URL");
        if (it != res.headers.end()) {
            packet.metadata["Application-URL"] = it->second;
            if (g_verbose) {
                std::cout << "[Metadata] DescriptionDownloader extracted Application-URL: " << it->second << "\n";
            }
        } else {
            if (g_verbose) {
                std::cout << "[Metadata] DescriptionDownloader found no Application-URL header\n";
            }
        }

        // Dispatch to analyzers (XmlAnalyzer will pick this up)
        m_dispatcher.Dispatch(packet, m_registry);
    }
}

} // namespace NetDiscovery
