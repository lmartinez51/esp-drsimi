/**
 * @file XmlAnalyzer.cpp
 * @brief UPnP device description XML analyzer implementation.
 */

#include "../include/XmlAnalyzer.h"
#include "../include/DeviceRegistry.h"
#include <iostream>
#include "../include/core/evidence/IdentityEvidence.h"

extern bool g_verbose;

namespace NetDiscovery {

void XmlAnalyzer::Analyze(const Packet& packet, DeviceRegistry& registry)
{
    if (packet.protocol != ProtocolType::XML && packet.protocol != ProtocolType::Unknown) {
        return; // Only process XML packets
    }

    std::string locationUrl;
    auto it = packet.metadata.find("LOCATION");
    if (it != packet.metadata.end()) {
        locationUrl = it->second;
    }

    std::string applicationUrl;
    auto appIt = packet.metadata.find("Application-URL");
    if (appIt != packet.metadata.end()) {
        applicationUrl = appIt->second;
        if (g_verbose) {
            std::cout << "[Metadata] XmlAnalyzer transferred Application-URL to UPnPEvidence: " << appIt->second << "\n";
        }
    } else {
        if (g_verbose) {
            std::cout << "[Metadata] XmlAnalyzer found no Application-URL in packet metadata\n";
        }
    }

    std::string xml = packet.rawPayload;
    if (xml.empty()) return;

    // A UPnP XML can contain a root device and multiple embedded devices.
    // We'll extract the root device first.
    std::string rootDeviceXml = ExtractElement(xml, "device");
    if (rootDeviceXml.empty()) return; // Not a valid UPnP description

    // To handle embedded devices, we find all <deviceList> elements
    // and parse each <device> within them.
    std::vector<IdentityEvidence> allDevices;
    
    // Parse the root device
    IdentityEvidence rootDevice;
    ParseDeviceNode(rootDeviceXml, locationUrl, applicationUrl, rootDevice);
    
    // Extract IP from location URL
    std::string ip;
    size_t schemeEnd = locationUrl.find("://");
    if (schemeEnd != std::string::npos) {
        size_t hostStart = schemeEnd + 3;
        size_t hostEnd = locationUrl.find(":", hostStart);
        if (hostEnd == std::string::npos) hostEnd = locationUrl.find("/", hostStart);
        if (hostEnd != std::string::npos) {
            ip = locationUrl.substr(hostStart, hostEnd - hostStart);
        }
    }
    rootDevice.ip = ip;
    
    allDevices.push_back(rootDevice);

    // Find embedded devices (this is a simplified extraction, assuming <deviceList> contains <device> tags)
    // In a real UPnP XML, embedded devices are within <deviceList> inside the parent <device>
    std::string deviceListXml = ExtractElement(rootDeviceXml, "deviceList");
    if (!deviceListXml.empty()) {
        std::vector<std::string> embeddedDeviceXmls = ExtractElements(deviceListXml, "device");
        for (const auto& edXml : embeddedDeviceXmls) {
            IdentityEvidence ed;
            ParseDeviceNode(edXml, locationUrl, applicationUrl, ed);
            ed.ip = ip;
            ed.protocolEvidence.upnp->locationUrl = locationUrl; // Same location URL as parent
            ed.parentUuid = rootDevice.uuid; // Set the parent UUID
            ed.rootUuid = rootDevice.uuid;
            ed.source = DiscoverySource::UPnP_XML;
            allDevices.push_back(ed);
        }
    }

    // Register all parsed devices
    for (const auto& dev : allDevices) {
        if (!dev.uuid.empty()) {
            registry.Register(dev);
        }
    }
}

void XmlAnalyzer::ParseDeviceNode(const std::string& deviceXml, const std::string& locationUrl, const std::string& applicationUrl, IdentityEvidence& dev) const
{
    std::string udn = ExtractElement(deviceXml, "UDN");
    dev.uuid = udn;
    
    dev.friendlyName = ExtractElement(deviceXml, "friendlyName");
    dev.manufacturer = ExtractElement(deviceXml, "manufacturer");
    // dev.manufacturerUrl = ExtractElement(deviceXml, "manufacturerURL"); // Handled in signature if needed
    dev.model = ExtractElement(deviceXml, "modelName");
    // dev.modelNumber = ExtractElement(deviceXml, "modelNumber");
    // dev.modelDescription = ExtractElement(deviceXml, "modelDescription");
    dev.serialNumber = ExtractElement(deviceXml, "serialNumber");
    dev.presentationUrl = ResolveUrl(locationUrl, ExtractElement(deviceXml, "presentationURL"));
    
    UPnPEvidence upnpEv;
    upnpEv.rawXml = deviceXml;
    upnpEv.locationUrl = locationUrl;
    upnpEv.applicationUrl = applicationUrl;
    upnpEv.services = ExtractServices(deviceXml, locationUrl);
    upnpEv.icons = ExtractIcons(deviceXml, locationUrl);
    upnpEv.deviceType = ExtractElement(deviceXml, "deviceType");
    
    dev.protocolEvidence.upnp = std::move(upnpEv);
    dev.source = DiscoverySource::UPnP_XML;
    
    dev.deviceTypes.push_back(ExtractElement(deviceXml, "deviceType"));
    for(const auto& srv : dev.protocolEvidence.upnp->services) {
        dev.services.push_back(srv.serviceType);
    }
}

std::string XmlAnalyzer::ExtractElement(const std::string& xml, const std::string& tagName) const
{
    // UPnP uses namespaces sometimes, e.g. <upnp:friendlyName>
    // A robust search should look for the tag name ending in > or matching xmlns prefix.
    // For this string scanner, we look for "<" + tagName + ">" or "<*:" + tagName + ">"
    
    // Simplification for the scanner: find "<tagName>" or ":tagName>"
    std::string openTag = "<" + tagName + ">";
    size_t start = xml.find(openTag);
    size_t contentStart = 0;
    if (start != std::string::npos) {
        contentStart = start + openTag.length();
    } else {
        // Try with namespace prefix, e.g. <ns:tagName>
        std::string tagEnd = ":" + tagName + ">";
        start = xml.find(tagEnd);
        if (start != std::string::npos) {
            // Find the opening '<' before the namespace
            size_t openBracket = xml.rfind('<', start);
            if (openBracket != std::string::npos) {
                contentStart = start + tagEnd.length();
            }
        }
    }
    
    if (contentStart == 0) return "";
    
    std::string closeTag = "</" + tagName + ">";
    size_t end = xml.find(closeTag, contentStart);
    if (end != std::string::npos) {
        return xml.substr(contentStart, end - contentStart);
    }
    
    // Check namespace close tag
    std::string closeTagNs = ":" + tagName + ">";
    size_t nsEnd = xml.find(closeTagNs, contentStart);
    if (nsEnd != std::string::npos) {
        size_t closeBracket = xml.rfind("</", nsEnd);
        if (closeBracket != std::string::npos && closeBracket >= contentStart) {
            return xml.substr(contentStart, closeBracket - contentStart);
        }
    }
    
    return "";
}

std::vector<std::string> XmlAnalyzer::ExtractElements(const std::string& xml, const std::string& tagName) const
{
    std::vector<std::string> results;
    std::string openTag = "<" + tagName + ">";
    std::string closeTag = "</" + tagName + ">";
    
    size_t pos = 0;
    while (true) {
        size_t start = xml.find(openTag, pos);
        if (start == std::string::npos) break;
        
        size_t contentStart = start + openTag.length();
        size_t end = xml.find(closeTag, contentStart);
        if (end == std::string::npos) break;
        
        results.push_back(xml.substr(contentStart, end - contentStart));
        pos = end + closeTag.length();
    }
    
    return results;
}

std::vector<UPnPService> XmlAnalyzer::ExtractServices(const std::string& xml, const std::string& locationUrl) const
{
    std::vector<UPnPService> services;
    std::string serviceListXml = ExtractElement(xml, "serviceList");
    if (serviceListXml.empty()) return services;
    
    std::vector<std::string> srvXmls = ExtractElements(serviceListXml, "service");
    for (const auto& srvXml : srvXmls) {
        UPnPService srv;
        srv.serviceType = ExtractElement(srvXml, "serviceType");
        srv.serviceId = ExtractElement(srvXml, "serviceId");
        srv.controlUrl = ResolveUrl(locationUrl, ExtractElement(srvXml, "controlURL"));
        srv.eventUrl = ResolveUrl(locationUrl, ExtractElement(srvXml, "eventSubURL"));
        srv.scpdUrl = ResolveUrl(locationUrl, ExtractElement(srvXml, "SCPDURL"));
        services.push_back(srv);
    }
    
    return services;
}

std::vector<UPnPIcon> XmlAnalyzer::ExtractIcons(const std::string& xml, const std::string& locationUrl) const
{
    std::vector<UPnPIcon> icons;
    std::string iconListXml = ExtractElement(xml, "iconList");
    if (iconListXml.empty()) return icons;
    
    std::vector<std::string> iconXmls = ExtractElements(iconListXml, "icon");
    for (const auto& iXml : iconXmls) {
        UPnPIcon icon;
        icon.mimetype = ExtractElement(iXml, "mimetype");
        
        std::string w = ExtractElement(iXml, "width");
        std::string h = ExtractElement(iXml, "height");
        std::string d = ExtractElement(iXml, "depth");
        
        if (!w.empty()) icon.width = static_cast<int>(std::strtol(w.c_str(), nullptr, 10));
        if (!h.empty()) icon.height = static_cast<int>(std::strtol(h.c_str(), nullptr, 10));
        if (!d.empty()) icon.depth = static_cast<int>(std::strtol(d.c_str(), nullptr, 10));
        
        icon.url = ResolveUrl(locationUrl, ExtractElement(iXml, "url"));
        icons.push_back(icon);
    }
    
    return icons;
}

std::string XmlAnalyzer::ResolveUrl(const std::string& baseUrl, const std::string& relativeUrl) const
{
    if (relativeUrl.empty()) return "";
    
    // If it's already an absolute URL, return it
    if (relativeUrl.find("http://") == 0 || relativeUrl.find("https://") == 0) {
        return relativeUrl;
    }
    
    // Otherwise, prepend the base URL (which should be the location URL of the device)
    // Extract everything up to the third slash (e.g., http://192.168.1.5:8080)
    size_t schemeEnd = baseUrl.find("://");
    if (schemeEnd == std::string::npos) return relativeUrl;
    
    size_t pathStart = baseUrl.find("/", schemeEnd + 3);
    std::string hostPort = (pathStart == std::string::npos) ? baseUrl : baseUrl.substr(0, pathStart);
    
    if (relativeUrl[0] == '/') {
        return hostPort + relativeUrl;
    } else {
        // Need to combine with the base path
        std::string basePath = (pathStart == std::string::npos) ? "/" : baseUrl.substr(pathStart);
        size_t lastSlash = basePath.rfind("/");
        if (lastSlash != std::string::npos) {
            basePath = basePath.substr(0, lastSlash + 1);
        } else {
            basePath = "/";
        }
        return hostPort + basePath + relativeUrl;
    }
}

} // namespace NetDiscovery
