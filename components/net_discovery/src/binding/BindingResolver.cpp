/**
 * @file BindingResolver.cpp
 * @brief Implementation of BindingResolver candidate retrieval component (v5.0.0 Architecture Phase 8.1).
 */

#include "binding/BindingResolver.h"
#include "esp_timer.h"

namespace NetDiscovery {
namespace Binding {

BindingCandidateSet BindingResolver::ResolveCandidates(const OperationId& operationId, const ProtocolBindingRegistrySnapshot& snapshot) const {
    uint64_t startUs = esp_timer_get_time();
    
    std::vector<ActionBinding> candidates;
    for (const auto& binding : snapshot.bindings) {
        if (binding.GetOperationId() == operationId) {
            candidates.push_back(binding);
        }
    }

    BindingCandidateSet candidateSet(operationId, std::move(candidates));
    candidateSet.filtersApplied.push_back("OperationIdExactMatch");
    candidateSet.resolutionTimeMs = static_cast<uint64_t>((esp_timer_get_time() - startUs) / 1000);
    candidateSet.diagnostics["SnapshotBindingCount"] = std::to_string(snapshot.GetBindingCount());
    
    return candidateSet;
}

BindingCandidateSet BindingResolver::ResolveCandidates(const OperationId& operationId, const ProtocolBindingRegistry& registry) const {
    // Obtain zero-lock snapshot from registry
    ProtocolBindingRegistrySnapshot snapshot = registry.GetSnapshot();
    return ResolveCandidates(operationId, snapshot);
}

BindingCandidateSet BindingResolver::ResolveForCapability(const std::string& capabilityId, const ProtocolBindingRegistrySnapshot& snapshot) const {
    uint64_t startUs = esp_timer_get_time();

    std::unordered_map<AdapterId, ProtocolAdapterDescriptor> adapterMap;
    for (const auto& adapter : snapshot.adapters) {
        adapterMap[adapter.adapterId] = adapter;
    }

    std::vector<ActionBinding> candidates;
    for (const auto& binding : snapshot.bindings) {
        auto it = adapterMap.find(binding.GetAdapterId());
        if (it != adapterMap.end()) {
            for (const auto& cap : it->second.supportedCapabilities) {
                if (cap == capabilityId) {
                    candidates.push_back(binding);
                    break;
                }
            }
        }
    }

    BindingCandidateSet candidateSet(capabilityId, std::move(candidates));
    candidateSet.filtersApplied.push_back("CapabilityIdMatch");
    candidateSet.resolutionTimeMs = static_cast<uint64_t>((esp_timer_get_time() - startUs) / 1000);
    candidateSet.diagnostics["CapabilityId"] = capabilityId;

    return candidateSet;
}

} // namespace Binding
} // namespace NetDiscovery
