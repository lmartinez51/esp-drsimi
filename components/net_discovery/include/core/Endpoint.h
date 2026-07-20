/**
 * @file Endpoint.h
 * @brief Network endpoint abstraction (address + port).
 *
 * Core layer — no protocol-specific dependencies.
 * Part of the NetDiscovery Core domain model.
 *
 * Replaces raw (ip, port) pairs in Packet, preparing the architecture
 * for IPv6, named addresses, and additional endpoint metadata.
 *
 * Portability: STL only — fully portable to ESP-IDF.
 */

#pragma once

#include <cstdint>
#include <string>

namespace NetDiscovery {

// ============================================================
// Endpoint
// ============================================================

/**
 * @brief Represents one end of a network communication channel.
 *
 * @note  address is intentionally a string rather than a uint32_t to
 *        support both IPv4 dotted-decimal ("192.168.1.1") and IPv6
 *        colon-hex notation without requiring a second representation.
 *
 * @note  For future extensibility, additional fields such as interface
 *        index, zone ID (for IPv6 link-local), or hostname may be added
 *        without breaking existing Packet users.
 */
struct Endpoint {
    /**
     * @brief Network address as a printable string.
     *
     * IPv4 example: "192.168.1.50"
     * IPv6 example: "fe80::1%eth0"
     * Empty string indicates address is unknown.
     */
    std::string address;

    /**
     * @brief UDP or TCP port number (host byte order).
     *
     * Zero indicates port is unknown or not applicable.
     */
    uint16_t port{0};

    // ----------------------------------------------------------------
    // Comparison (for use as map keys or in sets)
    // ----------------------------------------------------------------
    bool operator==(const Endpoint& other) const noexcept
    {
        return address == other.address && port == other.port;
    }

    bool operator!=(const Endpoint& other) const noexcept
    {
        return !(*this == other);
    }

    bool operator<(const Endpoint& other) const noexcept
    {
        if (address != other.address) return address < other.address;
        return port < other.port;
    }
};

} // namespace NetDiscovery
