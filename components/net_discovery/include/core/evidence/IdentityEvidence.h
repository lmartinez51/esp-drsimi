/**
 * @file IdentityEvidence.h
 * @brief Representation of raw, unmerged protocol data used for identity resolution.
 */

#pragma once

#include <string>
#include <vector>
#include "../DiscoverySource.h"
#include "../ProtocolEvidence.h"

namespace NetDiscovery {

/**
 * @brief Represents a single collection of identity data gathered from one protocol source.
 */
struct IdentityEvidence {
    // Basic network information
    std::string ip;
    
    // Core identity identifiers
    std::string uuid;
    std::string rootUuid;
    std::string parentUuid;
    
    // Descriptive metadata
    std::string manufacturer;
    std::string friendlyName;
    std::string model;
    std::string serialNumber;
    std::string presentationUrl;
    
    // Extracted capabilities and types
    std::vector<std::string> deviceTypes;
    std::vector<std::string> services;
    
    // Header/Raw info
    std::string serverHeader;
    
    // The source of this evidence (e.g. SSDP, UPnP_XML)
    DiscoverySource source{DiscoverySource::Unknown};
    
    // Original structured evidence if any
    ProtocolEvidence protocolEvidence;
};

} // namespace NetDiscovery
