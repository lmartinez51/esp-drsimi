/**
 * @file SSDPAnalyzer.cpp
 * @brief SSDP protocol analyzer implementation.
 *
 * Parses SSDP headers from Packet::rawPayload and updates DeviceRegistry.
 * No socket code. No printing. No filesystem access.
 * STL only for parsing — fully portable to ESP-IDF.
 */

#include "../include/SSDPAnalyzer.h"
#include "../include/PacketUtilities.h"
#include "../include/DeviceRegistry.h"
#include "../include/core/evidence/IdentityEvidence.h"

#include <algorithm>
#include <sstream>
#include <string>

namespace NetDiscovery {

// ============================================================
// Name()
// ============================================================

std::string SSDPAnalyzer::Name() const
{
    return "SSDP";
}

// ============================================================
// Analyze()
// ============================================================

void SSDPAnalyzer::Analyze(const Packet& packet, DeviceRegistry& registry)
{
    // Classify the packet. We process:
    //   - HttpResponse  (reply to our M-SEARCH)
    //   - Notify        (unsolicited advertisement during passive listen)
    // We skip M-SEARCH packets sent by other hosts on the network.
    const PacketType type = PacketUtilities::DetectType(packet.rawPayload);
    if (type == PacketType::MSearch || type == PacketType::Unknown) {
        return;
    }

    // --------------------------------------------------------
    // Extract key SSDP headers from the raw payload.
    // --------------------------------------------------------
    const std::string usn      = PacketUtilities::ExtractHeaderValue(packet.rawPayload, "USN");
    const std::string location = PacketUtilities::ExtractHeaderValue(packet.rawPayload, "LOCATION");
    const std::string server   = PacketUtilities::ExtractHeaderValue(packet.rawPayload, "SERVER");
    const std::string nts      = PacketUtilities::ExtractHeaderValue(packet.rawPayload, "NTS");
    // ST is present in HTTP Responses; NT is present in NOTIFY messages.
    const std::string st       = PacketUtilities::ExtractHeaderValue(packet.rawPayload, "ST");
    const std::string nt       = PacketUtilities::ExtractHeaderValue(packet.rawPayload, "NT");
    const std::string serviceType = st.empty() ? nt : st;

    // --------------------------------------------------------
    // Extract UUID — the primary registry key.
    // Determine whether this USN is a sub-service/device or a root device.
    // USN format:
    //   Root:    uuid:XXXX
    //   Sub:     uuid:XXXX::urn:schemas...:service:Name:1
    // --------------------------------------------------------
    const std::string rootUuidFromUsn = ExtractUuidFromUsn(usn);  // always the root uuid:XXXX
    if (rootUuidFromUsn == "unknown" || rootUuidFromUsn.empty()) {
        return;  // Cannot store without a key.
    }

    // If USN contains '::' it's a sub-device/service announcement.
    // We register under the root UUID and record the root for fusion.
    const auto sepPos = usn.find("::");
    const bool isSubDevice = (sepPos != std::string::npos);

    // --------------------------------------------------------
    // Build IdentityEvidence from the extracted information.
    // --------------------------------------------------------
    IdentityEvidence device;
    device.ip           = packet.source.address;
    device.uuid         = rootUuidFromUsn;  // always register under root UUID
    device.serverHeader = server;
    device.source       = DiscoverySource::SSDP;

    // If this is a sub-service/device USN, record root relationship.
    // (Not strictly needed since uuid == rootUuid, but kept for clarity.)
    if (isSubDevice) {
        device.rootUuid = rootUuidFromUsn;
    }

    UPnPEvidence upnpEv;
    upnpEv.locationUrl = location;

    // Add a Service/Device-type entry if this response advertises a specific type.
    if (!serviceType.empty()
        && serviceType != "ssdp:all"
        && serviceType != "upnp:rootdevice"
        && serviceType.rfind("uuid:", 0) != 0)   // skip bare UUID echoes
    {
        UPnPService svc;
        svc.serviceType = serviceType;
        // controlUrl, eventUrl, scpdUrl populated from XML.
        upnpEv.services.push_back(std::move(svc));

        device.services.push_back(serviceType);
        device.deviceTypes.push_back(serviceType);
    }

    device.protocolEvidence.upnp = std::move(upnpEv);

    // --------------------------------------------------------
    // Persist into the registry
    // --------------------------------------------------------
    registry.Register(device);
}

// ============================================================
// BuildMSearchRequest() — static
// ============================================================

std::string SSDPAnalyzer::BuildMSearchRequest(const std::string& multicastAddr,
                                               int                port,
                                               const std::string& searchTarget,
                                               int                mxSeconds)
{
    std::ostringstream oss;
    oss << "M-SEARCH * HTTP/1.1\r\n"
        << "HOST:" << multicastAddr << ":" << port << "\r\n"
        << "MAN:\"ssdp:discover\"\r\n"
        << "MX:"  << mxSeconds    << "\r\n"
        << "ST:"  << searchTarget << "\r\n"
        << "\r\n";
    return oss.str();
}

// ============================================================
// ExtractUuidFromUsn() — static
// ============================================================

std::string SSDPAnalyzer::ExtractUuidFromUsn(const std::string& usn)
{
    const auto uuidPos = usn.find("uuid:");
    if (uuidPos == std::string::npos) return "unknown";

    std::string fromUuid = usn.substr(uuidPos);

    // Trim after "::" (separates UUID from service URN).
    const auto sepPos = fromUuid.find("::");
    if (sepPos != std::string::npos) {
        fromUuid = fromUuid.substr(0, sepPos);
    }

    return fromUuid;  // e.g. "uuid:a1b2c3d4-e5f6-7890-abcd-ef1234567890"
}

// ============================================================
// ExtractUuid() — static
// ============================================================

std::string SSDPAnalyzer::ExtractUuid(const std::string& raw)
{
    const std::string usn = PacketUtilities::ExtractHeaderValue(raw, "USN");
    if (usn.empty()) return "unknown";
    return ExtractUuidFromUsn(usn);
}

} // namespace NetDiscovery
