/**
 * @file ProtocolEvidence.h
 * @brief Container for protocol-specific evidence.
 */

#pragma once

#include "evidence/UPnPEvidence.h"
#include <optional>

namespace NetDiscovery {

/**
 * @brief Structured evidence collected from various protocols.
 * 
 * Future protocols (BLE, mDNS, etc.) will add their own fields here.
 */
struct ProtocolEvidence {
    std::optional<UPnPEvidence> upnp;
};

} // namespace NetDiscovery
