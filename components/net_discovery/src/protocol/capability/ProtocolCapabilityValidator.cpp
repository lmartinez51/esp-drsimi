/**
 * @file ProtocolCapabilityValidator.cpp
 * @brief Implementation of ProtocolCapabilityValidator (v5.0.0 Architecture Phase 13).
 */

#include "protocol/capability/ProtocolCapabilityValidator.h"

namespace NetDiscovery {
namespace Protocol {

CapabilityValidationResult ProtocolCapabilityValidator::Validate(
        const Execution::ExecutionStep&        step,
        const ProtocolCapabilityRequirement&   requirement,
        const ProtocolCapabilitySet&           capabilitySet) const {

    if (requirement.IsEmpty()) {
        return CapabilityValidationResult(
            true, {}, {}, {"No capabilities required by step " + step.GetStepId()}, {});
    }

    bool isValid = true;
    std::vector<CapabilityId> missing;
    std::vector<std::string> warnings;
    std::vector<std::string> diagnostics;

    // Check required capabilities
    for (const auto& capId : requirement.GetRequiredCapabilities()) {
        if (!capabilitySet.Contains(capId)) {
            isValid = false;
            missing.push_back(capId);
            diagnostics.push_back("Step " + step.GetStepId() + " missing required protocol capability: " + capId);
        } else {
            diagnostics.push_back("Step " + step.GetStepId() + " satisfied capability: " + capId);
        }
    }

    // Check optional capabilities
    for (const auto& optCapId : requirement.GetOptionalCapabilities()) {
        if (!capabilitySet.Contains(optCapId)) {
            warnings.push_back("Step " + step.GetStepId() + " optional capability not supported: " + optCapId);
        }
    }

    return CapabilityValidationResult(isValid, missing, {}, warnings, diagnostics,
                                       {{"stepId", step.GetStepId()}, {"operationId", step.GetOperationId()}});
}

} // namespace Protocol
} // namespace NetDiscovery
