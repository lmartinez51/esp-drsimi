/**
 * @file BindingSelector.h
 * @brief Deterministic selection engine choosing optimal ActionBinding (v5.0.0 Architecture Phase 8.1).
 * 
 * BindingSelector consumes a BindingCandidateSet and a ProtocolBindingRegistrySnapshot.
 * Performs ZERO registry lookups, ZERO locking, and ZERO protocol execution.
 */

#pragma once

#include "binding/ActionBinding.h"
#include "binding/ProtocolAdapterDescriptor.h"
#include "binding/AdapterRuntimeState.h"
#include "binding/BindingCandidateSet.h"
#include "binding/BindingSelectionResult.h"
#include "binding/ProtocolBindingRegistrySnapshot.h"
#include "binding/ProtocolBindingRegistry.h"

#include <vector>
#include <string>

namespace NetDiscovery {
namespace Binding {

/**
 * @brief Selection engine evaluating candidate bindings against runtime states deterministically.
 */
class BindingSelector {
public:
    BindingSelector() = default;
    ~BindingSelector() = default;

    /**
     * @brief Selects optimal ActionBinding consuming a BindingCandidateSet and an immutable ProtocolBindingRegistrySnapshot.
     * 
     * Zero Lock Guarantee: Performs no registry lookups or thread locks during scoring.
     */
    BindingSelectionResult SelectBinding(const BindingCandidateSet& candidateSet, 
                                         const ProtocolBindingRegistrySnapshot& snapshot) const;

    /**
     * @brief Convenient overload taking operationId and ProtocolBindingRegistry.
     */
    BindingSelectionResult SelectBinding(const OperationId& operationId, const ProtocolBindingRegistry& registry) const;

    /**
     * @brief Direct evaluation overload consuming raw snapshot vectors.
     */
    BindingSelectionResult SelectBinding(const OperationId& operationId, 
                                         const std::vector<ActionBinding>& candidateBindings, 
                                         const std::vector<AdapterRuntimeState>& runtimeStates) const;

    /**
     * @brief Evaluates multi-dimensional BindingScore for a candidate binding given adapter runtime state.
     */
    BindingScore EvaluateScore(const ActionBinding& binding, const std::optional<AdapterRuntimeState>& runtimeState) const;
};

} // namespace Binding
} // namespace NetDiscovery
