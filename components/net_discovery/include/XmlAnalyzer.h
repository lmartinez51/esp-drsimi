/**
 * @file XmlAnalyzer.h
 * @brief UPnP device description XML analyzer.
 *
 * Implements IProtocolAnalyzer.
 * Extracts device metadata and service definitions from the UPnP XML document
 * fetched by the DescriptionDownloader. Populates the strongly-typed Device
 * C++ model without using heavy DOM trees.
 */

#pragma once

#include "IProtocolAnalyzer.h"
#include "core/evidence/IdentityEvidence.h"
#include <string>

namespace NetDiscovery {

class XmlAnalyzer : public IProtocolAnalyzer {
public:
    XmlAnalyzer() = default;
    ~XmlAnalyzer() override = default;

    std::string Name() const override { return "UPnP_XML"; }

    /**
     * @brief Analyze an XML Packet and update the DeviceRegistry.
     * 
     * Expects the Packet's payload to contain the UPnP Device Description XML.
     * Expects Packet::metadata["URL"] to contain the original LOCATION URL.
     * Expects Packet::metadata["UUID"] to contain the device UUID (optional).
     */
    void Analyze(const Packet& packet, DeviceRegistry& registry) override;

private:
    IdentityEvidence ParseDeviceDescription(const std::string& xml, const std::string& locationUrl) const;
    void ParseDeviceNode(const std::string& deviceXml, const std::string& locationUrl, const std::string& applicationUrl, IdentityEvidence& dev) const;
    
    std::string ExtractElement(const std::string& xml, const std::string& tagName) const;
    std::vector<std::string> ExtractElements(const std::string& xml, const std::string& tagName) const;
    
    std::vector<UPnPService> ExtractServices(const std::string& xml, const std::string& locationUrl) const;
    std::vector<UPnPIcon> ExtractIcons(const std::string& xml, const std::string& locationUrl) const;
    
    std::string ResolveUrl(const std::string& baseUrl, const std::string& relativeUrl) const;
};

} // namespace NetDiscovery
