/**
 * @file BindingResolver.h
 * @brief Subsystem component retrieving candidate bindings for an OperationDefinition (v5.0.0 Architecture Phase 8.1).
 * 
 * BindingResolver is strictly responsible for querying registry indexes and returning 
 * a BindingCandidateSet. It performs NO candidate scoring, ranking, or selection (which are 
 * owned exclusively by BindingSelector).
 */

#pragma once

#include "binding/ActionBinding.h"
#include "binding/BindingCandidateSet.h"
#include "binding/ProtocolBindingRegistrySnapshot.h"
#include "binding/ProtocolBindingRegistry.h"

#include <string>

namespace NetDiscovery {
namespace Binding {

/**
 * @brief Resolver component querying registry indexes and packaging BindingCandidateSet instances.
 */
class BindingResolver {
public:
    BindingResolver() = default;
    ~BindingResolver() = default;

    /**
     * @brief Resolves candidates for a target operationId using an immutable registry snapshot.
     */
    BindingCandidateSet ResolveCandidates(const OperationId& operationId, const ProtocolBindingRegistrySnapshot& snapshot) const;

    /**
     * @brief Resolves candidates for a target operationId by obtaining a lock-free snapshot from a registry.
     */
    BindingCandidateSet ResolveCandidates(const OperationId& operationId, const ProtocolBindingRegistry& registry) const;

    /**
     * @brief Resolves candidates for a target capabilityId using an immutable snapshot.
     */
    BindingCandidateSet ResolveForCapability(const std::string& capabilityId, const ProtocolBindingRegistrySnapshot& snapshot) const;
};

} // namespace Binding
} // namespace NetDiscovery
