/**
 * @file ProtocolNormalizer.cpp
 * @brief ProtocolNormalizer implementation.
 */

#include "../include/ProtocolNormalizer.h"
#include <algorithm>

namespace NetDiscovery {

void ProtocolNormalizer::Normalize(LogicalDevice& device) {
    device.normalizedServices.clear();
    
    auto addService = [&device](const std::string& name, const std::string& domain, const std::string& version, const std::string& endpoint) {
        // Prevent exact duplicates
        for (const auto& s : device.normalizedServices) {
            if (s.name == name && s.domain == domain && s.version == version) {
                return;
            }
        }
        
        NormalizedService ns;
        ns.name = name;
        ns.domain = domain;
        ns.version = version;
        ns.endpointUrl = endpoint;
        device.normalizedServices.push_back(std::move(ns));
    };
    
    // Iterate all protocol endpoints and normalize their evidence
    for (const auto& ep : device.endpoints) {
        if (ep.evidence.upnp.has_value()) {
            // Normalize UPnP Services from the <serviceList>
            for (const auto& svc : ep.evidence.upnp->services) {
                std::string type = svc.serviceType;
                std::string domain = "upnp";
                std::string name = type;
                std::string version = "1";

                // Parse ':service:Name:version' patterns
                auto pos = type.find(":service:");
                if (pos != std::string::npos) {
                    domain = type.substr(0, pos);
                    std::string remainder = type.substr(pos + 9);
                    auto verPos = remainder.rfind(':');
                    if (verPos != std::string::npos) {
                        name    = remainder.substr(0, verPos);
                        version = remainder.substr(verPos + 1);
                    } else {
                        name = remainder;
                    }
                    if (domain == "urn:schemas-upnp-org") {
                        domain = "upnp";
                    }
                }
                // Parse ':device:Name:version' patterns (device-type URNs)
                else {
                    auto dpos = type.find(":device:");
                    if (dpos != std::string::npos) {
                        domain = type.substr(0, dpos);
                        std::string remainder = type.substr(dpos + 8);
                        auto verPos = remainder.rfind(':');
                        if (verPos != std::string::npos) {
                            name    = remainder.substr(0, verPos);
                            version = remainder.substr(verPos + 1);
                        } else {
                            name = remainder;
                        }
                        if (domain == "urn:schemas-upnp-org") {
                            domain = "upnp";
                        }
                    }
                }

                addService(name, domain, version,
                           svc.controlUrl.empty() ? ep.evidence.upnp->locationUrl : svc.controlUrl);
            }

            // Also normalize the endpoint's own deviceType field (set by XmlAnalyzer)
            // so that device-type evidence is always available for role assignment.
            const std::string& dt = ep.evidence.upnp->deviceType;
            if (!dt.empty()) {
                std::string domain = "upnp";
                std::string name   = dt;
                std::string version = "1";
                auto dpos = dt.find(":device:");
                if (dpos != std::string::npos) {
                    domain = dt.substr(0, dpos);
                    std::string remainder = dt.substr(dpos + 8);
                    auto verPos = remainder.rfind(':');
                    if (verPos != std::string::npos) {
                        name    = remainder.substr(0, verPos);
                        version = remainder.substr(verPos + 1);
                    } else {
                        name = remainder;
                    }
                    if (domain == "urn:schemas-upnp-org") {
                        domain = "upnp";
                    }
                }
                addService(name, domain, version, ep.evidence.upnp->locationUrl);
            }
        }
    }
}

} // namespace NetDiscovery
