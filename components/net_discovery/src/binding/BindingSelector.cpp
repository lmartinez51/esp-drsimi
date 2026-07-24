/**
 * @file BindingSelector.cpp
 * @brief Implementation of deterministic BindingSelector engine (v5.0.0 Architecture Phase 8.1).
 */

#include "binding/BindingSelector.h"
#include "binding/BindingResolver.h"

#include <algorithm>
#include <sstream>

namespace NetDiscovery {
namespace Binding {

BindingScore BindingSelector::EvaluateScore(const ActionBinding& binding, const std::optional<AdapterRuntimeState>& runtimeState) const {
    BindingScore score;
    score.priorityScore = binding.GetPriority();

    if (runtimeState.has_value()) {
        const auto& state = runtimeState.value();
        
        // Health Scoring
        switch (state.healthState) {
            case AdapterHealthState::Healthy:  score.healthScore = 50; break;
            case AdapterHealthState::Degraded: score.healthScore = 20; break;
            case AdapterHealthState::Offline:  score.healthScore = -1000; break; // Disqualifying penalty
            case AdapterHealthState::Unknown:
            default:                            score.healthScore = -100; break;
        }

        // Availability Scoring
        switch (state.availability) {
            case AdapterAvailability::Available:   score.availabilityScore = 50; break;
            case AdapterAvailability::Busy:        score.availabilityScore = 10; break;
            case AdapterAvailability::Unavailable: score.availabilityScore = -500; break; // Disqualifying penalty
            default:                               score.availabilityScore = -50; break;
        }

        // Authentication Readiness Scoring
        if (!binding.RequiresAuthentication() && !state.authenticated) {
            score.authenticationScore = 10;
        } else if (state.authenticated) {
            score.authenticationScore = 10; // Auth token/key ready
        } else {
            score.authenticationScore = -30; // Auth required but key not set
        }
    } else {
        // Unregistered runtime state penalty
        score.healthScore = -200;
        score.availabilityScore = -200;
        score.authenticationScore = 0;
    }

    score.totalScore = score.priorityScore + score.healthScore + score.availabilityScore + score.authenticationScore;
    return score;
}

BindingSelectionResult BindingSelector::SelectBinding(const OperationId& operationId, const ProtocolBindingRegistry& registry) const {
    // 1. Obtain zero-lock point-in-time snapshot
    ProtocolBindingRegistrySnapshot snapshot = registry.GetSnapshot();

    // 2. Resolve candidate set via BindingResolver
    BindingResolver resolver;
    BindingCandidateSet candidateSet = resolver.ResolveCandidates(operationId, snapshot);

    // 3. Perform lock-free selection
    return SelectBinding(candidateSet, snapshot);
}

BindingSelectionResult BindingSelector::SelectBinding(const BindingCandidateSet& candidateSet,
                                                       const ProtocolBindingRegistrySnapshot& snapshot) const {
    return SelectBinding(candidateSet.operationId, candidateSet.bindings, snapshot.runtimeStates);
}

BindingSelectionResult BindingSelector::SelectBinding(const OperationId& operationId,
                                                       const std::vector<ActionBinding>& candidateBindings,
                                                       const std::vector<AdapterRuntimeState>& runtimeStates) const {
    BindingSelectionResult result;

    if (operationId.empty() || candidateBindings.empty()) {
        result.selectionReason = "No candidate bindings provided for operation: " + (operationId.empty() ? "<empty>" : operationId);
        return result;
    }

    // Fast O(1) map of runtime states
    std::unordered_map<AdapterId, AdapterRuntimeState> runtimeStateMap;
    for (const auto& state : runtimeStates) {
        runtimeStateMap[state.adapterId] = state;
    }

    struct EvaluatedCandidate {
        ActionBinding binding;
        BindingScore score;
    };

    std::vector<EvaluatedCandidate> evaluatedList;
    evaluatedList.reserve(candidateBindings.size());

    for (const auto& binding : candidateBindings) {
        std::optional<AdapterRuntimeState> stateOpt;
        auto it = runtimeStateMap.find(binding.GetAdapterId());
        if (it != runtimeStateMap.end()) {
            stateOpt = it->second;
        }

        BindingScore score = EvaluateScore(binding, stateOpt);
        // Only consider candidates with positive total score (filters out offline/unavailable adapters)
        if (score.totalScore > 0) {
            evaluatedList.push_back({binding, score});
        }
    }

    if (evaluatedList.empty()) {
        result.selectionReason = "All candidate bindings for operation '" + operationId + "' rejected due to offline/unavailable adapter runtime state";
        return result;
    }

    // Deterministic Sorting: Primary by totalScore descending, Secondary by BindingId ascending
    std::stable_sort(evaluatedList.begin(), evaluatedList.end(), [](const EvaluatedCandidate& a, const EvaluatedCandidate& b) {
        if (a.score.totalScore != b.score.totalScore) {
            return a.score.totalScore > b.score.totalScore;
        }
        return a.binding.GetBindingId() < b.binding.GetBindingId();
    });

    // Populate Selection Result
    result.selectedBinding = evaluatedList.front().binding;
    result.score = evaluatedList.front().score;

    std::ostringstream ss;
    ss << "Selected primary binding '" << result.selectedBinding->GetBindingId()
       << "' via protocol '" << result.selectedBinding->GetProtocol()
       << "' (Total Score: " << result.score.totalScore
       << " [Priority=" << result.score.priorityScore
       << ", Health=" << result.score.healthScore
       << ", Avail=" << result.score.availabilityScore
       << ", Auth=" << result.score.authenticationScore << "])";
    result.selectionReason = ss.str();

    // Populate fallbacks
    for (size_t i = 1; i < evaluatedList.size(); ++i) {
        result.fallbackCandidates.push_back(evaluatedList[i].binding);
    }

    return result;
}

} // namespace Binding
} // namespace NetDiscovery
