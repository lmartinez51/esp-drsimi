/**
 * @file PacketUtilities.h
 * @brief Generic packet analysis utilities (renamed from PacketAnalyzer).
 *
 * Utility layer — depends on Core types only (Packet, Header).
 *
 * The word "Analyzer" is reserved for IProtocolAnalyzer implementations.
 * This class provides stateless helper functions that are protocol-agnostic:
 *
 *   - Packet type detection from the first HTTP line
 *   - Generic header extraction from raw payloads (used by analyzers)
 *   - Timestamp formatting for display and capture files
 *   - Elapsed time formatting
 *
 * All methods are static. No instances are required.
 *
 * Portability: STL only — fully portable to ESP-IDF.
 */

#pragma once

#include "core/Packet.h"

#include <chrono>
#include <string>
#include <vector>

namespace NetDiscovery {

// ============================================================
// PacketType enum
// ============================================================

/**
 * @brief First-line classification of an HTTP-style packet.
 *
 * Derived by inspecting the opening line of the raw payload only.
 * Does not constitute a full protocol parse.
 */
enum class PacketType {
    HttpResponse,   ///< First line starts with "HTTP/" (SSDP M-SEARCH reply).
    Notify,         ///< First line starts with "NOTIFY" (unsolicited UPnP ad).
    MSearch,        ///< First line starts with "M-SEARCH" (discovery request).
    Unknown         ///< Unrecognized or malformed first line.
};

// ============================================================
// PacketUtilities
// ============================================================

/**
 * @brief Stateless generic packet helper functions.
 *
 * No state — all methods are static. No protocol-specific logic.
 */
class PacketUtilities {
public:
    // ----------------------------------------------------------------
    // Packet classification
    // ----------------------------------------------------------------

    /**
     * @brief Detect the packet type from the first HTTP-style line.
     *
     * Comparison is case-insensitive.
     *
     * @param raw  Complete raw packet text.
     * @return     Detected PacketType.
     */
    static PacketType DetectType(const std::string& raw);

    /**
     * @brief Convert a PacketType to a human-readable display string.
     */
    static std::string TypeToString(PacketType type);

    // ----------------------------------------------------------------
    // Raw payload header extraction
    // ----------------------------------------------------------------

    /**
     * @brief Extract the value of the first header matching @p name
     *        (case-insensitive) from a raw HTTP-style payload string.
     *
     * Scans line-by-line starting after the first (status/request) line.
     * Stops at the first blank line.
     *
     * For structured access on a pre-parsed Packet, use:
     *     GetHeaderValue(packet.headers, "LOCATION")
     *
     * @param raw         Complete raw payload text.
     * @param headerName  Header name to find (e.g. "LOCATION", "USN").
     * @return Header value with leading whitespace stripped, or "".
     */
    static std::string ExtractHeaderValue(const std::string& raw,
                                          const std::string& headerName);

    /**
     * @brief Parse all HTTP-style headers from a raw payload string
     *        into a vector of Header objects.
     *
     * Preserves original ordering and allows duplicate names.
     * Skips the first (status/request) line and stops at blank line.
     *
     * @param raw  Complete raw payload text.
     * @return     Ordered vector of Header structs.
     */
    static std::vector<Header> ParseHeaders(const std::string& raw);

    // ----------------------------------------------------------------
    // Timestamp and duration formatting
    // ----------------------------------------------------------------

    /**
     * @brief Format a specific system_clock time point as
     *        "YYYY-MM-DD HH:MM:SS.mmm" in local time.
     *
     * @param tp  Time point to format.
     * @return    Formatted timestamp string.
     */
    static std::string FormatTimestamp(
        const std::chrono::system_clock::time_point& tp);

    /**
     * @brief Format the current local time as "YYYY-MM-DD HH:MM:SS.mmm".
     */
    static std::string FormatTimestamp();

    /**
     * @brief Format a duration in milliseconds for human display.
     *
     * Examples: 450ms → "450ms",  2500ms → "2.5s".
     *
     * @param ms  Duration in milliseconds.
     * @return    Human-readable duration string.
     */
    static std::string FormatElapsed(long long ms);
};

} // namespace NetDiscovery
