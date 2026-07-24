/**
 * @file ProtocolCapabilityValidator.h
 * @brief Pure capability validation component (v5.0.0 Architecture Phase 13).
 */

#pragma once

#include "protocol/capability/CapabilityValidationResult.h"
#include "protocol/capability/ProtocolCapabilityRequirement.h"
#include "protocol/capability/ProtocolCapabilitySet.h"
#include "execution/ExecutionStep.h"

namespace NetDiscovery {
namespace Protocol {

/**
 * @brief Pure validator evaluating step capability requirements against adapter capabilities.
 *
 * Guaranteed Invariant: ProtocolCapabilityValidator performs ONLY validation.
 * Performs ZERO execution, opens ZERO sockets, invokes ZERO transports, and makes ZERO network calls.
 */
class ProtocolCapabilityValidator {
public:
    ProtocolCapabilityValidator() = default;

    /**
     * @brief Validates an ExecutionStep requirement against a ProtocolCapabilitySet.
     */
    CapabilityValidationResult Validate(
        const Execution::ExecutionStep&        step,
        const ProtocolCapabilityRequirement&   requirement,
        const ProtocolCapabilitySet&           capabilitySet) const;
};

} // namespace Protocol
} // namespace NetDiscovery
