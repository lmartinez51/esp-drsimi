#include "../include/DeviceFusionEngine.h"
#include "../include/core/StandardService.h"
#include "../include/core/ServiceDescriptor.h"
#include <iostream>
#include <algorithm>
#include <map>
#include <set>

namespace NetDiscovery {

std::vector<LogicalDevice> DeviceFusionEngine::Fuse(const std::vector<IdentityEvidence>& evidences, const IdentityScoringPolicy& policy) {
    // Step 1: Pre-process evidence to map child UUIDs to root UUIDs
    std::map<std::string, std::string> childToRoot;
    for (const auto& ev : evidences) {
        if (!ev.uuid.empty()) {
            if (!ev.parentUuid.empty()) {
                childToRoot[ev.uuid] = ev.parentUuid;
            } else if (!ev.rootUuid.empty() && ev.rootUuid != ev.uuid) {
                childToRoot[ev.uuid] = ev.rootUuid;
            }
        }
    }
    
    // Resolve chains: if A -> B and B -> C, then A -> C
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto& pair : childToRoot) {
            auto it = childToRoot.find(pair.second);
            if (it != childToRoot.end() && it->second != pair.second) {
                pair.second = it->second;
                changed = true;
            }
        }
    }
    
    // Normalize evidence by setting rootUuid if missing but known from mapping
    std::vector<IdentityEvidence> normalizedEvidences = evidences;
    for (auto& ev : normalizedEvidences) {
        if (ev.rootUuid.empty() && !ev.uuid.empty() && childToRoot.find(ev.uuid) != childToRoot.end()) {
            ev.rootUuid = childToRoot[ev.uuid];
        }
    }

    // Step 2: Group ALL evidence by UUID.
    // Multiple evidence records can share the same UUID (e.g. one SSDP entry
    // and one XML entry for uuid:X).  Merge them into a single per-UUID bucket
    // before reasoning about parent / child relationships.
    std::map<std::string, std::vector<IdentityEvidence>> byUuid;
    std::vector<IdentityEvidence> noUuidEv;   // evidence with no UUID at all

    for (const auto& ev : normalizedEvidences) {
        if (ev.uuid.empty()) {
            noUuidEv.push_back(ev);
        } else {
            byUuid[ev.uuid].push_back(ev);
        }
    }

    // Determine parent UUID for each UUID bucket.
    // Take the first non-empty parentUuid found across the bucket's evidences.
    auto getParentForUuid = [&](const std::string& uuid) -> std::string {
        auto it = byUuid.find(uuid);
        if (it == byUuid.end()) return {};
        for (const auto& ev : it->second) {
            if (!ev.parentUuid.empty()) return ev.parentUuid;
        }
        // Fall back to rootUuid (but only if it differs from the UUID itself)
        for (const auto& ev : it->second) {
            if (!ev.rootUuid.empty() && ev.rootUuid != uuid) return ev.rootUuid;
        }
        return {};
    };

    // Build a parent→children map at the UUID level.
    std::map<std::string, std::vector<std::string>> uuidChildren;
    std::set<std::string> childUuids;

    for (const auto& kv : byUuid) {
        std::string parent = getParentForUuid(kv.first);
        if (!parent.empty() && parent != kv.first) {
            uuidChildren[parent].push_back(kv.first);
            childUuids.insert(kv.first);
        }
    }

    // Collect root UUIDs (those not listed as a child of anyone).
    std::vector<std::string> rootUuids;
    for (const auto& kv : byUuid) {
        if (childUuids.find(kv.first) == childUuids.end()) {
            rootUuids.push_back(kv.first);
        }
    }

    // Build device groups: each root UUID + all its descendants (BFS).
    std::vector<std::vector<IdentityEvidence>> deviceGroups;

    for (const auto& rootUuid : rootUuids) {
        std::vector<IdentityEvidence> group;

        std::vector<std::string> queue = {rootUuid};
        while (!queue.empty()) {
            std::string cur = queue.front();
            queue.erase(queue.begin());

            // Add all evidence records for this UUID.
            auto it = byUuid.find(cur);
            if (it != byUuid.end()) {
                for (const auto& ev : it->second) {
                    group.push_back(ev);
                }
            }

            // Recurse into children.
            auto childIt = uuidChildren.find(cur);
            if (childIt != uuidChildren.end()) {
                for (const auto& child : childIt->second) {
                    queue.push_back(child);
                }
            }
        }

        if (!group.empty()) {
            deviceGroups.push_back(std::move(group));
        }
    }

    // Add any no-UUID evidence as lone groups.
    for (const auto& ev : noUuidEv) {
        deviceGroups.push_back({ev});
    }

    // Step 3: Convert device groups to LogicalDevices.
    // The richest evidence record in each group seeds the LogicalDevice metadata.
    std::vector<LogicalDevice> baseDevices;
    for (const auto& group : deviceGroups) {
        if (group.empty()) continue;

        LogicalDevice newDev;

        // Prefer the first non-empty UUID and IP seen in the group.
        for (const auto& ev : group) {
            if (newDev.id.empty() && !ev.uuid.empty())       newDev.id = ev.uuid;
            if (newDev.primaryIp.empty() && !ev.ip.empty())  newDev.primaryIp = ev.ip;
        }
        if (newDev.id.empty()) newDev.id = "unknown_" + newDev.primaryIp;

        for (const auto& ev : group) {
            DiscoverySource src = ev.source;
            if (newDev.displayName.empty() && !ev.friendlyName.empty()) {
                newDev.displayName = ev.friendlyName;
                newDev.provenance.Track("displayName", src);
            }
            if (newDev.manufacturer.empty() && !ev.manufacturer.empty()) {
                newDev.manufacturer = ev.manufacturer;
                newDev.provenance.Track("manufacturer", src);
            }
            if (newDev.model.empty() && !ev.model.empty()) {
                newDev.model = ev.model;
                newDev.provenance.Track("model", src);
            }
            if (newDev.signature.serialNumber.empty() && !ev.serialNumber.empty()) {
                newDev.signature.serialNumber = ev.serialNumber;
                newDev.provenance.Track("serialNumber", src);
            }
            if (newDev.signature.presentationUrl.empty() && !ev.presentationUrl.empty()) {
                newDev.signature.presentationUrl = ev.presentationUrl;
                newDev.provenance.Track("presentationUrl", src);
            }
        } // End of first loop

        std::map<std::string, ProtocolEndpoint> epMap;
        for (const auto& ev : group) {
            auto& endpoint = epMap[ev.ip];
            if (endpoint.ip.empty()) {
                endpoint.ip = ev.ip;
                endpoint.uuid = ev.uuid;
            }
            if (endpoint.serverHeader.empty() && !ev.serverHeader.empty()) {
                endpoint.serverHeader = ev.serverHeader;
            }
            // Add discovery source if not already present
            if (std::find(endpoint.discoverySources.begin(), endpoint.discoverySources.end(), ev.source) == endpoint.discoverySources.end()) {
                endpoint.discoverySources.push_back(ev.source);
            }
            
            // Merge UPnP Evidence
            if (ev.protocolEvidence.upnp.has_value()) {
                if (!endpoint.evidence.upnp.has_value()) {
                    endpoint.evidence.upnp = ev.protocolEvidence.upnp;
                } else {
                    auto& existUpnp = endpoint.evidence.upnp.value();
                    const auto& liveUpnp = ev.protocolEvidence.upnp.value();
                    if (existUpnp.applicationUrl.empty()) existUpnp.applicationUrl = liveUpnp.applicationUrl;
                    if (existUpnp.locationUrl.empty()) existUpnp.locationUrl = liveUpnp.locationUrl;
                    if (existUpnp.deviceType.empty()) existUpnp.deviceType = liveUpnp.deviceType;
                    
                    for (const auto& svc : liveUpnp.services) {
                        bool found = false;
                        for (const auto& eSvc : existUpnp.services) {
                            if (eSvc.serviceType == svc.serviceType) { found = true; break; }
                        }
                        if (!found) existUpnp.services.push_back(svc);
                    }
                    for (const auto& ico : liveUpnp.icons) {
                        bool found = false;
                        for (const auto& eIco : existUpnp.icons) {
                            if (eIco.url == ico.url) { found = true; break; }
                        }
                        if (!found) existUpnp.icons.push_back(ico);
                    }
                }
            }
        }
        
        for (auto& pair : epMap) {
            newDev.endpoints.push_back(std::move(pair.second));
        }
        baseDevices.push_back(std::move(newDev));
    }
    
    // Helper: extract the primary LOCATION URL from a LogicalDevice's endpoints.
    auto getLocationUrl = [](const LogicalDevice& dev) -> std::string {
        for (const auto& ep : dev.endpoints) {
            if (ep.evidence.upnp.has_value() && !ep.evidence.upnp->locationUrl.empty()) {
                return ep.evidence.upnp->locationUrl;
            }
        }
        return {};
    };

    // Helper: extract Base URL (scheme://host:port) from a location URL
    auto getBaseUrl = [](const std::string& url) -> std::string {
        if (url.empty()) return {};
        size_t schemeEnd = url.find("://");
        if (schemeEnd == std::string::npos) return {};
        size_t pathStart = url.find('/', schemeEnd + 3);
        if (pathStart == std::string::npos) return url;
        return url.substr(0, pathStart);
    };

    // Step 4: Heuristic Fusion across LogicalDevices
    std::vector<LogicalDevice> logicalDevices;
    for (auto& baseDev : baseDevices) {
        bool fused = false;
        const std::string baseLocation = getLocationUrl(baseDev);

        for (auto& logicalDev : logicalDevices) {
            int fusionScore = 0;

            // 1. Exact Serial Number Match
            if (!baseDev.signature.serialNumber.empty() && baseDev.signature.serialNumber == logicalDev.signature.serialNumber) {
                fusionScore += policy.exactSerialNumberMatch;
            }

            // 2. UUID Match
            if (!baseDev.id.empty() && logicalDev.id == baseDev.id) {
                fusionScore += policy.sameUuidMatch;
            }

            // 3. Same LOCATION URL (strongest non-UUID heuristic for SSDP sub-devices)
            const std::string logicalLocation = getLocationUrl(logicalDev);
            if (!baseLocation.empty() && !logicalLocation.empty()) {
                if (baseLocation == logicalLocation) {
                    fusionScore += policy.sameLocationUrlMatch;
                } else {
                    std::string baseBaseUrl = getBaseUrl(baseLocation);
                    std::string logicalBaseUrl = getBaseUrl(logicalLocation);
                    if (!baseBaseUrl.empty() && baseBaseUrl == logicalBaseUrl) {
                        fusionScore += policy.sameBaseUrlMatch;
                    }
                }
            }

            // 4. Presentation URL Match
            if (!baseDev.signature.presentationUrl.empty() && baseDev.signature.presentationUrl == logicalDev.signature.presentationUrl) {
                fusionScore += policy.samePresentationUrlMatch;
            }

            // 5. IP Match & Canonical Physical Device Heuristic
            if (!baseDev.primaryIp.empty() && baseDev.primaryIp == logicalDev.primaryIp) {
                fusionScore += policy.sameIpMatch;
                // If same IP AND (same manufacturer OR same model OR same friendly name), boost score to guarantee physical fusion
                if (!baseDev.manufacturer.empty() && baseDev.manufacturer == logicalDev.manufacturer) {
                    fusionScore += policy.sameManufacturerMatch + 30;
                }
                if (!baseDev.model.empty() && baseDev.model == logicalDev.model) {
                    fusionScore += policy.sameModelMatch + 30;
                }
                if (!baseDev.displayName.empty() && baseDev.displayName == logicalDev.displayName) {
                    fusionScore += policy.sameFriendlyNameMatch + 30;
                }
            }

            // 6. Friendly Name Match
            if (!baseDev.displayName.empty() && baseDev.displayName == logicalDev.displayName) {
                fusionScore += policy.sameFriendlyNameMatch;
            }

            // 7. Manufacturer Match
            if (!baseDev.manufacturer.empty() && baseDev.manufacturer == logicalDev.manufacturer) {
                fusionScore += policy.sameManufacturerMatch;
            }

            // 8. Model Match
            if (!baseDev.model.empty() && baseDev.model == logicalDev.model) {
                fusionScore += policy.sameModelMatch;
            }

            if (fusionScore >= policy.fusionThreshold) {
                // Merge endpoints without duplicates
                for (auto& ep : baseDev.endpoints) {
                    bool epExists = false;
                    for (auto& existingEp : logicalDev.endpoints) {
                        if (existingEp.ip == ep.ip) {
                            if (existingEp.serverHeader.empty()) existingEp.serverHeader = ep.serverHeader;
                            if (!existingEp.evidence.upnp.has_value() && ep.evidence.upnp.has_value()) {
                                existingEp.evidence.upnp = ep.evidence.upnp;
                            } else if (existingEp.evidence.upnp.has_value() && ep.evidence.upnp.has_value()) {
                                auto& existUpnp = existingEp.evidence.upnp.value();
                                const auto& liveUpnp = ep.evidence.upnp.value();
                                if (existUpnp.applicationUrl.empty()) existUpnp.applicationUrl = liveUpnp.applicationUrl;
                                if (existUpnp.locationUrl.empty()) existUpnp.locationUrl = liveUpnp.locationUrl;
                                for (const auto& s : liveUpnp.services) {
                                    bool sFound = false;
                                    for (const auto& eSvc : existUpnp.services) {
                                        if (eSvc.serviceType == s.serviceType) { sFound = true; break; }
                                    }
                                    if (!sFound) existUpnp.services.push_back(s);
                                }
                            }
                            epExists = true;
                            break;
                        }
                    }
                    if (!epExists) {
                        logicalDev.endpoints.push_back(std::move(ep));
                    }
                }
                
                if (logicalDev.displayName.empty() && !baseDev.displayName.empty()) {
                    logicalDev.displayName = baseDev.displayName;
                }
                if (logicalDev.manufacturer.empty() && !baseDev.manufacturer.empty()) {
                    logicalDev.manufacturer = baseDev.manufacturer;
                }
                if (logicalDev.model.empty() && !baseDev.model.empty()) {
                    logicalDev.model = baseDev.model;
                }
                if (logicalDev.signature.serialNumber.empty() && !baseDev.signature.serialNumber.empty()) {
                    logicalDev.signature.serialNumber = baseDev.signature.serialNumber;
                }
                if (logicalDev.signature.presentationUrl.empty() && !baseDev.signature.presentationUrl.empty()) {
                    logicalDev.signature.presentationUrl = baseDev.signature.presentationUrl;
                }
                
                fused = true;
                break;
            }
        }
        
        if (!fused) {
            logicalDevices.push_back(std::move(baseDev));
        }
    }
    
    // Step 5: Extract and normalize services into the generic ServiceDescriptor format
    for (auto& dev : logicalDevices) {
        for (const auto& ep : dev.endpoints) {
            if (!ep.evidence.upnp.has_value()) continue;
            
            for (const auto& upnpSvc : ep.evidence.upnp->services) {
                // Deduplicate by serviceId across endpoints
                bool exists = false;
                for (const auto& existing : dev.services) {
                    if (existing.serviceId == upnpSvc.serviceId) {
                        exists = true;
                        break;
                    }
                }
                if (exists) continue;

                ServiceDescriptor sd;
                sd.protocolFamily = "UPnP";
                sd.uniqueId = dev.id + "::" + upnpSvc.serviceId;
                sd.serviceType = upnpSvc.serviceType;
                sd.serviceId = upnpSvc.serviceId;
                sd.controlUrl = upnpSvc.controlUrl;
                sd.eventUrl = upnpSvc.eventUrl;
                sd.scpdUrl = upnpSvc.scpdUrl;
                
                // Map to StandardService
                if (sd.serviceType.find("RenderingControl") != std::string::npos) {
                    sd.standardType = StandardService::RenderingControl;
                } else if (sd.serviceType.find("AVTransport") != std::string::npos) {
                    sd.standardType = StandardService::AVTransport;
                } else if (sd.serviceType.find("ConnectionManager") != std::string::npos) {
                    sd.standardType = StandardService::ConnectionManager;
                } else if (sd.serviceType.find("DIAL") != std::string::npos) {
                    sd.standardType = StandardService::DIAL;
                } else if (sd.serviceType.find("RemoteControl") != std::string::npos || sd.serviceType.find("NetworkControl") != std::string::npos) {
                    sd.standardType = StandardService::RemoteControl;
                } else {
                    sd.standardType = StandardService::Unknown;
                }
                
                dev.services.push_back(sd);
            }
        }
    }
    
    return logicalDevices;
}

} // namespace NetDiscovery
