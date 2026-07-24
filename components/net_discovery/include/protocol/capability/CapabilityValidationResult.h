/**
 * @file CapabilityValidationResult.h
 * @brief Immutable outcome of a ProtocolCapabilityValidator check (v5.0.0 Architecture Phase 13.1).
 */

#pragma once

#include "protocol/capability/ProtocolCapability.h"

#include <vector>
#include <string>
#include <unordered_map>
#include <utility>

namespace NetDiscovery {
namespace Protocol {

/**
 * @brief Immutable outcome of a capability validation check performed before step execution.
 */
struct CapabilityValidationResult {
    bool                      valid{true};
    std::vector<CapabilityId> missingCapabilities;
    std::vector<CapabilityId> unsatisfiedDependencies;
    std::vector<std::string>  warnings;
    std::vector<std::string>  diagnostics;
    std::unordered_map<std::string, std::string> metadata;

    CapabilityValidationResult() = default;

    CapabilityValidationResult(bool isValid,
                               std::vector<CapabilityId> missing,
                               std::vector<CapabilityId> unsatisfiedDeps = {},
                               std::vector<std::string> warn = {},
                               std::vector<std::string> diag = {},
                               std::unordered_map<std::string, std::string> meta = {})
        : valid(isValid)
        , missingCapabilities(std::move(missing))
        , unsatisfiedDependencies(std::move(unsatisfiedDeps))
        , warnings(std::move(warn))
        , diagnostics(std::move(diag))
        , metadata(std::move(meta)) {}

    bool IsValid() const { return valid; }
    bool HasMissingCapabilities() const { return !missingCapabilities.empty(); }
    bool HasUnsatisfiedDependencies() const { return !unsatisfiedDependencies.empty(); }
    const std::vector<CapabilityId>& GetMissingCapabilities() const { return missingCapabilities; }
    const std::vector<CapabilityId>& GetUnsatisfiedDependencies() const { return unsatisfiedDependencies; }
    const std::vector<std::string>& GetDiagnostics() const { return diagnostics; }

    std::string GetSummary() const {
        if (valid) return "Validation Passed";
        std::string summary = "Missing capabilities: ";
        for (std::size_t i = 0; i < missingCapabilities.size(); ++i) {
            if (i > 0) summary += ", ";
            summary += missingCapabilities[i];
        }
        return summary;
    }
};

} // namespace Protocol
} // namespace NetDiscovery
