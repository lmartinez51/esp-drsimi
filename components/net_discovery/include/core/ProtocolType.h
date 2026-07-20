/**
 * @file ProtocolType.h
 * @brief Enumeration of application-layer protocol types.
 *
 * Core layer — no protocol-specific dependencies.
 * Part of the NetDiscovery Core domain model.
 *
 * A Packet carries its detected ProtocolType so that the
 * AnalyzerDispatcher can route efficiently, and callers can inspect
 * the protocol without parsing the raw payload.
 *
 * Portability: Pure enum + constexpr — fully portable to ESP-IDF.
 */

#pragma once

namespace NetDiscovery {

// ============================================================
// ProtocolType
// ============================================================

/**
 * @brief Application-layer protocol identified in a Packet.
 *
 * Most packets will initially carry ProtocolType::Unknown (if detection
 * has not been run) or ProtocolType::SSDP.
 *
 * Set by PacketUtilities::DetectProtocol() or populated directly by
 * the transport layer when the protocol is known by construction
 * (e.g. an M-SEARCH response is always SSDP).
 */
enum class ProtocolType : unsigned char {
    Unknown,        ///< Protocol not yet identified.
    SSDP,           ///< Simple Service Discovery Protocol (UPnP).
    HTTP,           ///< Hypertext Transfer Protocol.
    SOAP,           ///< Simple Object Access Protocol (UPnP control).
    XML,            ///< Raw XML document exchange.
    JSON,           ///< JSON-encoded payload.
    WebSocket,      ///< WebSocket upgrade (RFC 6455).
    DLNA,           ///< Digital Living Network Alliance extensions.
    SamsungRemote,  ///< Samsung TV remote control protocol.
    BLE,            ///< Bluetooth Low Energy advertisement.
    mDNS,           ///< Multicast DNS (RFC 6762).
};

/**
 * @brief Return a human-readable string for a ProtocolType value.
 * @param p  Protocol to describe.
 * @return   Short protocol name string.
 */
constexpr const char* ToString(ProtocolType p) noexcept
{
    switch (p) {
        case ProtocolType::SSDP:          return "SSDP";
        case ProtocolType::HTTP:          return "HTTP";
        case ProtocolType::SOAP:          return "SOAP";
        case ProtocolType::XML:           return "XML";
        case ProtocolType::JSON:          return "JSON";
        case ProtocolType::WebSocket:     return "WebSocket";
        case ProtocolType::DLNA:          return "DLNA";
        case ProtocolType::SamsungRemote: return "SamsungRemote";
        case ProtocolType::BLE:           return "BLE";
        case ProtocolType::mDNS:          return "mDNS";
        default:                          return "Unknown";
    }
}

} // namespace NetDiscovery
