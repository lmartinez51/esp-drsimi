/**
 * @file TransportProtocol.h
 * @brief Enumeration of physical transport protocols.
 *
 * Core layer — no protocol-specific dependencies.
 * Part of the NetDiscovery Core domain model.
 *
 * Portability: Pure enum + constexpr — fully portable to ESP-IDF.
 */

#pragma once

namespace NetDiscovery {

// ============================================================
// TransportProtocol
// ============================================================

/**
 * @brief Physical transport used to carry a Packet.
 *
 * Stored on every Packet so that analyzers and dispatchers can make
 * routing decisions without inspecting the raw payload.
 */
enum class TransportProtocol : unsigned char {
    Unknown,    ///< Transport could not be determined.
    UDP,        ///< User Datagram Protocol (connectionless).
    TCP,        ///< Transmission Control Protocol (stream-oriented).
    Multicast,  ///< UDP multicast (e.g. 239.255.255.250:1900 for SSDP).
};

/**
 * @brief Return a human-readable string for a TransportProtocol value.
 * @param p  Protocol to describe.
 * @return   One of: "UDP", "TCP", "Multicast", "Unknown".
 */
constexpr const char* ToString(TransportProtocol p) noexcept
{
    switch (p) {
        case TransportProtocol::UDP:       return "UDP";
        case TransportProtocol::TCP:       return "TCP";
        case TransportProtocol::Multicast: return "Multicast";
        default:                           return "Unknown";
    }
}

} // namespace NetDiscovery
