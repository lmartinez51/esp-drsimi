/**
 * @file Device.h
 * @brief Canonical representation of a discovered network device.
 *
 * Core layer — no protocol-specific dependencies.
 * Part of the NetDiscovery Core domain model.
 *
 * Multiple discovery protocols may update the same Device object.
 * DeviceRegistry handles merge semantics.
 */

#pragma once

#include "Capability.h"
#include "ControllerCandidate.h"
#include "DeviceFingerprint.h"
#include "DiscoveryEvidence.h"
#include "DiscoverySource.h"
#include "ProtocolEvidence.h"

#include <string>
#include <vector>

namespace NetDiscovery {

/**
 * @brief Current lifecycle state of a discovered device.
 */
enum class DeviceState {
    Unknown,
    Alive,
    ByeBye
};

/**
 * @brief Canonical representation of a discovered network device.
 *
 * A Device is protocol-agnostic. It can be discovered via SSDP, mDNS,
 * BLE, or any future protocol. 
 */
struct Device {
    // ----------------------------------------------------------------
    // Network identity
    // ----------------------------------------------------------------

    /// Primary IPv4 or IPv6 address string.
    std::string ip;

    /// Unique Device Name — "uuid:xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx".
    std::string uuid;
    
    // ----------------------------------------------------------------
    // Lifecycle & Confidence
    // ----------------------------------------------------------------
    
    /// Current lifecycle state.
    DeviceState state{DeviceState::Unknown};
    
    /// Confidence score (0-100) indicating reliability of the discovery data.
    int confidence{0};

    // ----------------------------------------------------------------
    // Human-readable identification & metadata
    // ----------------------------------------------------------------

    std::string friendlyName;
    std::string manufacturer;
    std::string manufacturerUrl;
    std::string model;         
    std::string modelNumber;
    std::string modelDescription;
    std::string serialNumber;
    std::string presentationUrl;

    // ----------------------------------------------------------------
    // Capabilities & Controllers
    // ----------------------------------------------------------------

    /// High-level functional capabilities.
    std::vector<Capability> capabilities;
    
    /// List of evaluated controller candidates.
    std::vector<ControllerCandidate> controllerCandidates;

    /// The name of the preferred controller (highest score).
    std::string preferredController;

    // ----------------------------------------------------------------
    // Discovery metadata
    // ----------------------------------------------------------------

    /// Server header string reported in HTTP/SSDP responses.
    std::string server;

    /// How this device was discovered.
    std::vector<DiscoverySource> discoverySources;

    // ----------------------------------------------------------------
    // Fingerprint & Evidence
    // ----------------------------------------------------------------
    
    /// Aggregated fingerprint used by ControllerResolver.
    DeviceFingerprint fingerprint;
    
    /// Evidence collected from multiple discovery protocols (for active/passive).
    std::vector<DiscoveryEvidence> evidence;

    /// Detailed, structured protocol-specific evidence.
    ProtocolEvidence protocolEvidence;
};

} // namespace NetDiscovery
