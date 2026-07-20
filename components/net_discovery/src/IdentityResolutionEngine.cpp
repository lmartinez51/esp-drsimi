#include "../include/IdentityResolutionEngine.h"
#include "../include/DeviceFusionEngine.h"
#include <algorithm>
#include <set>

namespace NetDiscovery {

IdentityResolutionEngine::IdentityResolutionEngine(IdentityScoringPolicy policy)
    : m_policy(policy)
{
}

std::vector<LogicalDevice> IdentityResolutionEngine::Resolve(const std::vector<IdentityEvidence>& evidences) const
{
    // Step 1: Use the fusion strategy to cluster evidence into LogicalDevices
    std::vector<LogicalDevice> devices = DeviceFusionEngine::Fuse(evidences, m_policy);
    
    // Step 2: Normalize signatures and compute confidence
    for (auto& dev : devices) {
        BuildDeviceSignature(dev);
        ComputeIdentityConfidence(dev);
    }
    
    return devices;
}

void IdentityResolutionEngine::ComputeIdentityConfidence(LogicalDevice& device) const
{
    device.confidenceBreakdown.clear();
    int confidenceScore = 0;
    
    auto hasMeaningfulContent = [](const std::string& str) {
        return !str.empty() && !std::all_of(str.begin(), str.end(), [](unsigned char c) { return std::isspace(c); });
    };
    
    // Evaluate Manufacturer
    if (hasMeaningfulContent(device.signature.manufacturer)) {
        confidenceScore += 30;
        device.confidenceBreakdown.push_back({"Manufacturer", 30});
    }
    
    // Evaluate Model
    if (hasMeaningfulContent(device.signature.model)) {
        confidenceScore += 25;
        device.confidenceBreakdown.push_back({"Model", 25});
    }
    
    // Evaluate Serial
    if (hasMeaningfulContent(device.signature.serialNumber)) {
        confidenceScore += 20;
        device.confidenceBreakdown.push_back({"Serial", 20});
    }
    
    // Evaluate FriendlyName
    if (hasMeaningfulContent(device.signature.friendlyName)) {
        confidenceScore += 15;
        device.confidenceBreakdown.push_back({"FriendlyName", 15});
    }
    
    // Evaluate PresentationURL
    if (hasMeaningfulContent(device.signature.presentationUrl)) {
        confidenceScore += 10;
        device.confidenceBreakdown.push_back({"PresentationURL", 10});
    }
    
    // Cap at 100
    device.confidence = std::min(100, confidenceScore);
}

void IdentityResolutionEngine::BuildDeviceSignature(LogicalDevice& device) const
{
    // Ensure all strings map to signature
    device.signature.manufacturer = device.manufacturer;
    device.signature.model = device.model;
    device.signature.friendlyName = device.displayName;
    
    std::set<std::string> ips;
    std::set<DiscoverySource> sources;
    std::set<std::string> namespaces;
    std::set<std::string> deviceTypes;
    std::set<std::string> serviceTypes;
    
    for (const auto& ep : device.endpoints) {
        if (!ep.ip.empty()) {
            ips.insert(ep.ip);
        }
        for (const auto& src : ep.discoverySources) {
            sources.insert(src);
        }
        
        if (ep.evidence.upnp.has_value()) {
            if (!ep.evidence.upnp->deviceType.empty()) {
                deviceTypes.insert(ep.evidence.upnp->deviceType);
                
                // Extract namespace
                auto pos = ep.evidence.upnp->deviceType.find(":device:");
                if (pos != std::string::npos) {
                    namespaces.insert(ep.evidence.upnp->deviceType.substr(0, pos));
                }
            }
            
            for (const auto& svc : ep.evidence.upnp->services) {
                serviceTypes.insert(svc.serviceType);
                
                // Extract namespace
                auto pos = svc.serviceType.find(":service:");
                if (pos != std::string::npos) {
                    namespaces.insert(svc.serviceType.substr(0, pos));
                }
            }
        }
    }
    
    device.signature.ips.assign(ips.begin(), ips.end());
    device.signature.discoverySources.assign(sources.begin(), sources.end());
    device.signature.namespaces.assign(namespaces.begin(), namespaces.end());
    device.signature.deviceTypes.assign(deviceTypes.begin(), deviceTypes.end());
    device.signature.serviceTypes.assign(serviceTypes.begin(), serviceTypes.end());
}

} // namespace NetDiscovery
