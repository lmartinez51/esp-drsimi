/**
 * @file Packet.h
 * @brief Generic, protocol-independent network packet model.
 *
 * Core layer — depends only on other Core types (Endpoint, Header,
 * TransportProtocol, ProtocolType). Has no protocol-specific logic.
 *
 * A Packet is the universal data carrier passed between the transport
 * layer (UdpSocket, MulticastSocket) and the protocol analysis layer
 * (SSDPAnalyzer, HTTPAnalyzer, etc.).
 *
 * Design notes:
 *   - `rawPayload` is the authoritative source of truth for the received
 *      bytes. All parsed fields (headers, body) are derived from it.
 *   - `headers` uses std::vector<Header> to preserve ordering and allow
 *      duplicate header names (HTTP semantics).
 *   - `source` / `destination` use Endpoint structs to prepare for IPv6.
 *   - `metadata` is an escape hatch for protocol-specific extras that
 *      do not belong in the fixed fields.
 *
 * Portability: STL only — fully portable to ESP-IDF.
 */

#pragma once

#include "Endpoint.h"
#include "Header.h"
#include "ProtocolType.h"
#include "TransportProtocol.h"

#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

namespace NetDiscovery {

// ============================================================
// Packet
// ============================================================

/**
 * @brief Protocol-independent representation of one received network packet.
 *
 * Created by the transport layer (UdpSocket, MulticastSocket) from a raw
 * socket receive. Passed unchanged to the AnalyzerDispatcher, which routes
 * it to one or more IProtocolAnalyzer implementations.
 */
struct Packet {
    // ----------------------------------------------------------------
    // Timing
    // ----------------------------------------------------------------

    /**
     * @brief Wall-clock timestamp of when this packet was received.
     *
     * Set to std::chrono::system_clock::now() immediately after recvfrom()
     * returns a positive byte count. Not the kernel timestamp.
     */
    std::chrono::system_clock::time_point timestamp{};

    // ----------------------------------------------------------------
    // Transport classification
    // ----------------------------------------------------------------

    /// Physical transport layer that delivered this packet.
    TransportProtocol transport{TransportProtocol::Unknown};

    /// Application-layer protocol identified in this packet.
    /// Set by PacketUtilities::DetectProtocol() or by the transport
    /// layer when the protocol is known by construction.
    ProtocolType protocol{ProtocolType::Unknown};

    // ----------------------------------------------------------------
    // Addressing (Endpoint = address + port)
    // ----------------------------------------------------------------

    /// Who sent this packet.
    Endpoint source;

    /// Where this packet was sent to (multicast group address for SSDP).
    Endpoint destination;

    // ----------------------------------------------------------------
    // Parsed content (populated by analyzers, not transport layer)
    // ----------------------------------------------------------------

    /**
     * @brief Parsed HTTP-style headers in original receive order.
     *
     * Populated by a protocol analyzer (e.g. SSDPAnalyzer) after
     * parsing rawPayload. Empty when the packet is first created by
     * the transport layer.
     *
     * Use GetHeaderValue(headers, "LOCATION") for lookups.
     */
    std::vector<Header> headers;

    /**
     * @brief Body content (everything after the blank separator line).
     *
     * For SSDP datagrams this is typically empty. For HTTP responses
     * it contains the entity body (e.g. XML device description).
     */
    std::string body;

    // ----------------------------------------------------------------
    // Raw payload (authoritative)
    // ----------------------------------------------------------------

    /**
     * @brief Complete raw bytes received from the socket, as a string.
     *
     * This is the authoritative source. All other fields are derived
     * from this. It is always populated by the transport layer.
     */
    std::string rawPayload;

    // ----------------------------------------------------------------
    // Protocol-specific extras
    // ----------------------------------------------------------------

    /**
     * @brief Arbitrary key-value metadata for protocol-specific extras.
     *
     * Analyzers may store protocol-specific fields here when no
     * standard Packet field applies. Keys are protocol-namespaced by
     * convention, e.g. "ssdp.searchTarget", "http.statusCode".
     */
    std::unordered_map<std::string, std::string> metadata;
};

} // namespace NetDiscovery
