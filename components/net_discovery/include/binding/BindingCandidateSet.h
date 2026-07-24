/**
 * @file BindingCandidateSet.h
 * @brief Candidate binding container output produced by BindingResolver (v5.0.0 Architecture Phase 8.1).
 * 
 * Replaces raw std::vector<ActionBinding> vectors with an extensible result structure 
 * containing operation details, filtered candidates, resolution latency, and diagnostics.
 */

#pragma once

#include "binding/ActionBinding.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace NetDiscovery {
namespace Binding {

/**
 * @brief Extensible candidate binding set returned by BindingResolver.
 */
struct BindingCandidateSet {
    std::string operationId;                                       // Target semantic operation ID
    std::vector<ActionBinding> bindings;                           // Filtered candidate ActionBindings
    uint64_t resolutionTimeMs{0};                                  // Query resolution latency in ms
    std::vector<std::string> filtersApplied;                       // List of resolution filters applied
    std::unordered_map<std::string, std::string> diagnostics;      // Extensible diagnostic metadata

    BindingCandidateSet() = default;

    BindingCandidateSet(std::string opId, std::vector<ActionBinding> candidateBindings)
        : operationId(std::move(opId)), bindings(std::move(candidateBindings)) {}

    bool Empty() const { return bindings.empty(); }
    size_t Size() const { return bindings.size(); }
};

} // namespace Binding
} // namespace NetDiscovery
