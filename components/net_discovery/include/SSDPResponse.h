/**
 * @file SSDPResponse.h
 * @brief Data model for a raw SSDP discovery response.
 *
 * SSDPResponse captures everything received from a single UDP datagram
 * returned during an M-SEARCH operation. Phase 1 only populates
 * remoteIp and rawResponse. All other fields are reserved for Phase 2
 * (SSDP parser).
 *
 * Phase status:
 *   - Phase 1 : remoteIp + rawResponse populated.
 *   - Phase 2 : All header fields parsed and populated.
 */

#pragma once

#include <string>
#include <unordered_map>

namespace NetDiscovery {

/**
 * @brief Represents a single SSDP HTTP/1.1 response datagram.
 *
 * Returned by SSDPClient::Discover() as part of a vector. Each
 * instance corresponds to exactly one UDP packet received from one
 * device on the network.
 */
struct SSDPResponse {
    // ----------------------------------------------------------------
    // Phase 1 fields — populated in Phase 1
    // ----------------------------------------------------------------

    /// Source IPv4 address of the responding device (e.g. "192.168.1.50").
    std::string remoteIp;

    /// Complete raw HTTP response text as received from the UDP socket.
    std::string rawResponse;

    // ----------------------------------------------------------------
    // Phase 2 fields — populated by the SSDP parser (Phase 2)
    // ----------------------------------------------------------------

    /**
     * @brief Absolute URL to the UPnP device description XML.
     *
     * Extracted from the LOCATION header. This URL will be fetched
     * by HttpClient in Phase 3 to retrieve the device description.
     */
    std::string location;

    /**
     * @brief Server identification string.
     *
     * Extracted from the SERVER header. Typically contains OS and
     * UPnP version information (e.g. "Linux/3.14 UPnP/1.0 Sonos/...").
     */
    std::string server;

    /**
     * @brief Unique Service Name.
     *
     * Extracted from the USN header. Uniquely identifies the device
     * and the service being advertised.
     */
    std::string usn;

    /**
     * @brief Search Target that matched the M-SEARCH request.
     *
     * Extracted from the ST header. For a wildcard search (ST:ssdp:all)
     * this will contain the specific service type of the responder.
     */
    std::string st;

    /**
     * @brief Cache-Control directive.
     *
     * Extracted from the CACHE-CONTROL header. Indicates how long the
     * advertisement is valid (e.g. "max-age=1800").
     */
    std::string cacheControl;

    /**
     * @brief Map of ALL headers present in the SSDP response.
     *
     * Keys are stored in lowercase for case-insensitive lookup.
     * Populated by the SSDP parser in Phase 2.
     */
    std::unordered_map<std::string, std::string> headers;
};

} // namespace NetDiscovery
