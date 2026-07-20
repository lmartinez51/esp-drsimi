#include "../include/PresentationFormatter.h"
#include <iostream>
#include <iomanip>
#include <set>

namespace NetDiscovery {

std::string PresentationFormatter::CapabilityToString(Capability cap) {
    // Delegate to the canonical ToString() defined in Capability.h so there
    // is exactly one place to maintain this mapping.
    return ToString(cap);
}


std::string PresentationFormatter::DiscoverySourceToString(DiscoverySource source) {
    switch (source) {
        case DiscoverySource::SSDP: return "SSDP Discovery";
        case DiscoverySource::UPnP_XML: return "UPnP XML Description";
        case DiscoverySource::mDNS: return "mDNS Discovery";
        default: return "Unknown Source";
    }
}

void PresentationFormatter::PrintLogicalDevices(const std::vector<LogicalDevice>& devices) {
    std::cout << "======================================================================\n";
    std::cout << "  DEVICE INTELLIGENCE ENGINE — Logical Devices (" << devices.size() << ")\n";
    std::cout << "======================================================================\n\n";

    for (const auto& dev : devices) {
        std::string title = dev.displayName;
        if (title.empty()) {
            if (!dev.manufacturer.empty() && !dev.model.empty()) {
                title = dev.manufacturer + " " + dev.model;
            } else if (!dev.manufacturer.empty()) {
                title = dev.manufacturer + " Device";
            } else {
                title = "Unknown Device (" + dev.primaryIp + ")";
            }
        }
        
        std::cout << "  " << title << "\n\n";
        
        // Identity Confidence
        std::cout << "  Identity Confidence: " << dev.confidence << "%\n";
        for (const auto& kv : dev.confidenceBreakdown) {
            std::cout << "    +" << kv.second << " " << kv.first << "\n";
        }
        std::cout << "\n";
        
        // Evidence Signature
        std::cout << "  Device Signature Evidence\n";
        if (!dev.signature.manufacturer.empty()) std::cout << "  - Manufacturer : " << dev.signature.manufacturer << "\n";
        if (!dev.signature.model.empty()) std::cout << "  - Model        : " << dev.signature.model << "\n";
        if (!dev.signature.friendlyName.empty()) std::cout << "  - FriendlyName : " << dev.signature.friendlyName << "\n";
        if (!dev.signature.serialNumber.empty()) std::cout << "  - Serial       : " << dev.signature.serialNumber << "\n";
        if (!dev.signature.presentationUrl.empty()) std::cout << "  - Presentation : " << dev.signature.presentationUrl << "\n";
        
        std::cout << "  - Primary Class: " << ToString(dev.primaryClass) << "\n";
        if (!dev.roles.empty()) {
            std::cout << "  - Roles        : ";
            for (size_t i = 0; i < dev.roles.size(); ++i) {
                std::cout << ToString(dev.roles[i]);
                if (i < dev.roles.size() - 1) std::cout << ", ";
            }
            std::cout << "\n";
        }
        std::cout << "\n";

        // Capabilities
        std::cout << "  Capabilities\n";
        if (dev.capabilities.empty()) {
            std::cout << "  (None)\n";
        } else {
            for (auto cap : dev.capabilities) {
                std::cout << "  ✓ " << CapabilityToString(cap) << "\n";
            }
        }
        std::cout << "\n";

        // Actions
        std::cout << "  Supported Actions\n";
        if (dev.actions.empty()) {
            std::cout << "  (None)\n";
        } else {
            for (const auto& action : dev.actions) {
                std::cout << "    - " << ToString(action.id) << "\n";
                if (!action.supportedParameters.empty()) {
                    std::cout << "(";
                    for (size_t i = 0; i < action.supportedParameters.size(); ++i) {
                        std::cout << action.supportedParameters[i].name;
                        if (i < action.supportedParameters.size() - 1) std::cout << ", ";
                    }
                    std::cout << ")";
                }
                std::cout << "\n";
            }
        }
        std::cout << "\n";

        // Protocol Endpoints
        std::cout << "  Protocol Endpoints\n";
        std::set<std::string> printedEndpoints;
        for (const auto& ep : dev.endpoints) {
            bool hasSsdp = std::find(ep.discoverySources.begin(), ep.discoverySources.end(), DiscoverySource::SSDP) != ep.discoverySources.end();
            bool hasUpnp = ep.evidence.upnp.has_value() && !ep.evidence.upnp->deviceType.empty();
            
            if (hasSsdp || (!hasSsdp && !hasUpnp && ep.discoverySources.empty())) {
                std::string epStr = "  - SSDP Discovery (" + ep.ip + ")";
                if (printedEndpoints.insert(epStr).second) {
                    std::cout << epStr << "\n";
                }
            }

            if (hasUpnp) {
                std::string type = ep.evidence.upnp->deviceType;
                auto pos = type.find(":device:");
                if (pos != std::string::npos) {
                    type = type.substr(pos + 8);
                    auto pos2 = type.find(":");
                    if (pos2 != std::string::npos) {
                        type = type.substr(0, pos2);
                    }
                }
                std::string epStr = "  - UPnP " + type + " (" + ep.ip + ")";
                if (printedEndpoints.insert(epStr).second) {
                    std::cout << epStr << "\n";
                }
            } else if (!hasSsdp) { // fallback for unknown
                std::string sourceStr = DiscoverySourceToString(ep.discoverySources.empty() ? DiscoverySource::SSDP : ep.discoverySources[0]);
                std::string epStr = "  - " + sourceStr + " (" + ep.ip + ")";
                if (printedEndpoints.insert(epStr).second) {
                    std::cout << epStr << "\n";
                }
            }
        }
        std::cout << "\n";

        // Generic Services
        std::cout << "  Exposed Services (" << dev.services.size() << ")\n";
        if (dev.services.empty()) {
            std::cout << "  (None)\n";
        } else {
            for (const auto& svc : dev.services) {
                std::cout << "  - " << svc.serviceType << "\n";
                if (!svc.controlUrl.empty()) {
                    std::cout << "      Control: " << svc.controlUrl << "\n";
                }
            }
        }
        std::cout << "\n";

        if (!dev.controllerCandidates.empty()) {
            std::cout << "  Controller Candidates\n";
            
            bool foundWinner = false;
            for (const auto& cand : dev.controllerCandidates) {
                if (!cand.isRejected && !foundWinner) {
                    std::cout << "  > [PREFERRED] " << cand.name << " (Confidence: " << cand.confidence << ")\n";
                    for (const auto& kv : cand.scoreBreakdown) {
                        std::cout << "      +" << kv.second << " " << kv.first << "\n";
                    }
                    foundWinner = true;
                } else if (!cand.isRejected) {
                    std::cout << "  ✓ [ACCEPTED] " << cand.name << " (Confidence: " << cand.confidence << ")\n";
                    for (const auto& kv : cand.scoreBreakdown) {
                        std::cout << "      +" << kv.second << " " << kv.first << "\n";
                    }
                } else {
                    std::cout << "  ✗ [REJECTED] " << cand.name << "\n";
                    std::cout << "      Reason: " << cand.diagnosticReason << "\n";
                }
            }
        }
        std::cout << "\n----------------------------------------------------------------------\n\n";
    }
}

} // namespace NetDiscovery
