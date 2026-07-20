/**
 * @file SSDPAnalyzer.h
 * @brief SSDP protocol analyzer — IProtocolAnalyzer implementation.
 *
 * Protocol layer — depends on IProtocolAnalyzer (protocol layer) and
 * Core types (Packet, Device, Service). No transport-layer dependency.
 *
 * Responsibilities:
 *   - Parse SSDP headers from Packet::rawPayload.
 *   - Extract UUID, LOCATION, SERVER, ST/NT headers.
 *   - Populate Service entries from the advertised service type.
 *   - Call DeviceRegistry::Register() with the extracted Device.
 *   - Provide BuildMSearchRequest() for SSDPClient to use.
 *
 * This class does NOT print anything. All output is handled by main().
 * This class does NOT send or receive network data.
 *
 * Portability: Depends only on IProtocolAnalyzer and Core — fully
 * portable to ESP-IDF.
 */

#pragma once

#include "IProtocolAnalyzer.h"
#include "core/Device.h"

#include <string>

namespace NetDiscovery {

// ============================================================
// SSDPAnalyzer
// ============================================================

/**
 * @brief SSDP protocol analyzer.
 *
 * Accepts any Packet whose rawPayload looks like an SSDP datagram
 * (HTTP Response, NOTIFY, or M-SEARCH) and updates the DeviceRegistry.
 */
class SSDPAnalyzer final : public IProtocolAnalyzer {
public:
    SSDPAnalyzer() = default;

    // ----------------------------------------------------------------
    // IProtocolAnalyzer interface
    // ----------------------------------------------------------------

    /**
     * @brief Return the analyzer name: "SSDP".
     */
    std::string Name() const override;

    /**
     * @brief Parse the SSDP packet and register the discovered device.
     *
     * Parses:
     *   - USN header → UUID (primary key)
     *   - LOCATION header → device.locationUrl
     *   - SERVER header → device.server
     *   - ST header (response) or NT header (NOTIFY) → service type
     *
     * Skips packets without a parseable UUID (cannot key into registry).
     * Handles NOTIFY announcements as well as HTTP responses.
     * Silently ignores M-SEARCH packets sent by other hosts.
     *
     * @param packet    Received SSDP datagram (read-only).
     * @param registry  Registry to update.
     */
    void Analyze(const Packet& packet, DeviceRegistry& registry) override;

    // ----------------------------------------------------------------
    // Static utilities used by SSDPClient
    // ----------------------------------------------------------------

    /**
     * @brief Build an SSDP M-SEARCH request string.
     *
     * The request conforms to UPnP Device Architecture 1.1 / RFC 2616.
     * Lines are terminated with CRLF; the request ends with a blank line.
     *
     * @param multicastAddr  Destination multicast address (e.g. "239.255.255.250").
     * @param port           Destination port (e.g. 1900).
     * @param searchTarget   ST header value (e.g. "ssdp:all").
     * @param mxSeconds      MX header value (response delay hint, ≥ 1).
     * @return               Ready-to-send M-SEARCH datagram string.
     */
    static std::string BuildMSearchRequest(const std::string& multicastAddr,
                                            int                port,
                                            const std::string& searchTarget,
                                            int                mxSeconds);

    // ----------------------------------------------------------------
    // Static extraction utilities (public for testing)
    // ----------------------------------------------------------------

    /**
     * @brief Extract the UUID portion from a raw USN header value.
     *
     * USN format: "uuid:xxxxxx-...-xxxx::urn:..."
     * Returns the "uuid:xxxxxx-...-xxxx" part (before "::").
     *
     * @param usn  Raw USN header value.
     * @return     UUID string, or "unknown" if not found.
     */
    static std::string ExtractUuidFromUsn(const std::string& usn);

    /**
     * @brief Extract UUID directly from a raw SSDP packet.
     *
     * Reads the USN header from rawPayload then calls ExtractUuidFromUsn().
     *
     * @param raw  Raw SSDP packet text.
     * @return     UUID string, or "unknown".
     */
    static std::string ExtractUuid(const std::string& raw);
};

} // namespace NetDiscovery
