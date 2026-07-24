/**
 * @file DeviceIdentityDescriptor.h
 * @brief Immutable aggregate combining DeviceIdentity and DeviceIdentityContext (v5.0.0 Architecture Phase 18).
 */

#pragma once

#include "identity/DeviceIdentity.h"
#include "identity/DeviceIdentityContext.h"

namespace NetDiscovery {
namespace Identity {

/**
 * @brief Immutable composite descriptor representing a logical device view.
 */
struct DeviceIdentityDescriptor {
    DeviceIdentity        identity;
    DeviceIdentityContext context;

    DeviceIdentityDescriptor() = default;

    DeviceIdentityDescriptor(DeviceIdentity id, DeviceIdentityContext ctx)
        : identity(std::move(id)), context(std::move(ctx)) {}

    bool IsValid() const { return identity.IsValid(); }
};

} // namespace Identity
} // namespace NetDiscovery
