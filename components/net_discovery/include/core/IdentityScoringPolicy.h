/**
 * @file IdentityScoringPolicy.h
 * @brief Configurable weights for identity resolution.
 */

#pragma once

namespace NetDiscovery {

/**
 * @brief Structure containing all weights used by the IdentityResolutionEngine.
 */
struct IdentityScoringPolicy {
    int exactSerialNumberMatch{100};
    int sameRootUuidMatch{80};
    int sameUuidMatch{70};
    int sameLocationUrlMatch{70};   ///< Same UPnP LOCATION URL = same physical device
    int sameBaseUrlMatch{70};       ///< Same UPnP Base URL (Host:Port) = same physical device
    int samePresentationUrlMatch{60};
    int sameFriendlyNameMatch{50};
    int sameManufacturerMatch{30};
    int sameModelMatch{30};
    int sameDeviceTypeMatch{20};
    int sameIpMatch{15};
    int sameServerHeaderMatch{10};

    // The threshold above which endpoints are considered the same physical device.
    int fusionThreshold{80};
};

} // namespace NetDiscovery
