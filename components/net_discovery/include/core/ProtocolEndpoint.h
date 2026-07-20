/**
 * @file ProtocolEndpoint.h
 * @brief Lightweight representation of a protocol interface for a LogicalDevice.
 */

#pragma once

#include "DiscoverySource.h"
#include "ProtocolEvidence.h"

#include <string>
#include <vector>

namespace NetDiscovery {

struct ProtocolEndpoint {
    std::string uuid;
    std::string ip;
    std::string serverHeader;
    std::vector<DiscoverySource> discoverySources;
    ProtocolEvidence evidence;
};

} // namespace NetDiscovery
