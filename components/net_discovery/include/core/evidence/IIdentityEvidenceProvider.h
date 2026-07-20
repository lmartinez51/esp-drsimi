/**
 * @file IIdentityEvidenceProvider.h
 * @brief Interface for protocol-specific evidence providers.
 */

#pragma once

#include "IdentityEvidence.h"
#include <vector>

namespace NetDiscovery {

/**
 * @brief Interface for any component that can provide identity evidence.
 * 
 * Future implementations could include BLEScanner, MDNSResolver, etc.
 */
class IIdentityEvidenceProvider {
public:
    virtual ~IIdentityEvidenceProvider() = default;

    /**
     * @brief Collect evidence from this provider.
     */
    virtual std::vector<IdentityEvidence> GetEvidence() const = 0;
};

} // namespace NetDiscovery
