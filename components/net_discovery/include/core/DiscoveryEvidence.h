/**
 * @file DiscoveryEvidence.h
 * @brief Representation of raw evidence collected during discovery.
 *
 * Core layer — no protocol-specific dependencies.
 * Part of the NetDiscovery Core domain model.
 *
 * Evidence accumulates over time from various protocols (SSDP, mDNS, HTTP, etc.)
 * rather than overwriting previous information. This allows intelligent downstream
 * processing (e.g. by ControllerResolver) to inspect the complete history of what
 * a device has claimed.
 *
 * Portability: STL only — fully portable to ESP-IDF.
 */

#pragma once

#include <string>

namespace NetDiscovery {

/**
 * @brief Raw evidence collected from a protocol.
 */
struct DiscoveryEvidence {
    /// The protocol or component that provided the evidence (e.g., "SSDP", "UPnP_XML", "DIAL").
    std::string source;

    /// The key or type of evidence (e.g., "Server", "ST", "friendlyName").
    std::string key;

    /// The raw value observed.
    std::string value;
};

} // namespace NetDiscovery
