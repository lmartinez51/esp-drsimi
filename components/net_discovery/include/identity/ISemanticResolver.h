/**
 * @file ISemanticResolver.h
 * @brief Pure semantic resolution interface for querying logical identities (v5.0.0 Architecture Phase 18).
 */

#pragma once

#include "identity/DeviceIdentityDescriptor.h"
#include <vector>
#include <string>

namespace NetDiscovery {
namespace Identity {

/**
 * @brief Pure abstract interface for semantic device queries.
 *
 * Contains ZERO NLP and ZERO LLM logic. Pure structural query interface.
 */
class ISemanticResolver {
public:
    virtual ~ISemanticResolver() = default;

    virtual std::vector<DeviceIdentityDescriptor> ResolveByCategory(const std::string& category) const = 0;
    virtual std::vector<DeviceIdentityDescriptor> ResolveByRoom(const std::string& room) const = 0;
    virtual std::vector<DeviceIdentityDescriptor> ResolveByAlias(const std::string& alias) const = 0;
};

} // namespace Identity
} // namespace NetDiscovery
