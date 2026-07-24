/**
 * @file UPnPSoapSerializer.cpp
 * @brief Implementation of UPnPSoapSerializer (v5.0.0 Architecture Phase 11.1).
 */

#include "protocol/upnp/UPnPSoapSerializer.h"

#include <sstream>

namespace NetDiscovery {
namespace Protocol {
namespace UPnP {

std::string UPnPSoapSerializer::EscapeXml(const std::string& input) const {
    std::ostringstream escaped;
    for (char c : input) {
        switch (c) {
            case '&':  escaped << "&amp;";  break;
            case '<':  escaped << "&lt;";   break;
            case '>':  escaped << "&gt;";   break;
            case '"':  escaped << "&quot;"; break;
            case '\'': escaped << "&apos;"; break;
            default:   escaped << c;        break;
        }
    }
    return escaped.str();
}

std::string UPnPSoapSerializer::SerializeEnvelope(
        const std::string& serviceType,
        const std::string& actionName,
        const std::unordered_map<std::string, std::string>& arguments) const {

    std::ostringstream xml;
    xml << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\r\n";
    xml << "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n";
    xml << "  <s:Body>\r\n";
    xml << "    <u:" << actionName << " xmlns:u=\"" << serviceType << "\">\r\n";

    for (const auto& kv : arguments) {
        xml << "      <" << kv.first << ">" << EscapeXml(kv.second) << "</" << kv.first << ">\r\n";
    }

    xml << "    </u:" << actionName << ">\r\n";
    xml << "  </s:Body>\r\n";
    xml << "</s:Envelope>";

    return xml.str();
}

} // namespace UPnP
} // namespace Protocol
} // namespace NetDiscovery
