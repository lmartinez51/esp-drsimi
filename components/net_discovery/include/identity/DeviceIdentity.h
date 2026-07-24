/**
 * @file DeviceIdentity.h
 * @brief Immutable identity object for logical device representations (v5.0.0 Architecture Phase 18).
 */

#pragma once

#include <string>
#include <cstdint>
#include <utility>

namespace NetDiscovery {
namespace Identity {

using IdentityId = std::string;

/**
 * @brief Immutable logical device identity. Zero runtime state.
 */
struct DeviceIdentity {
    IdentityId identityId;
    uint64_t   creationTimestampMs{0};
    uint32_t   version{1};
    std::string deviceCategory; ///< e.g. "TV", "Light", "Speaker", "Sensor"
    std::string manufacturer;
    std::string model;

    DeviceIdentity() = default;

    DeviceIdentity(IdentityId id,
                   std::string category,
                   std::string mfr = "",
                   std::string mdl = "",
                   uint64_t createdMs = 0,
                   uint32_t ver = 1)
        : identityId(std::move(id))
        , creationTimestampMs(createdMs)
        , version(ver)
        , deviceCategory(std::move(category))
        , manufacturer(std::move(mfr))
        , model(std::move(mdl)) {}

    bool IsValid() const { return !identityId.empty(); }
};

} // namespace Identity
} // namespace NetDiscovery
