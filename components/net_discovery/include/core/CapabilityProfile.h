#pragma once

#include "ActionId.h"
#include "Capability.h"
#include <string>
#include <vector>
#include <cstdint>

namespace NetDiscovery {

/**
 * @brief Explicit support state for incremental learning.
 */
enum class SupportState {
    Unknown,
    Supported,
    Unsupported
};

/**
 * @brief Bitmask representing constraints on action execution.
 */
enum class ExecutionConstraint : uint32_t {
    None                         = 0,
    RequiresWakeOnLAN            = 1 << 0,
    RequiresPowerOn              = 1 << 1,
    RequiresAuthenticatedSession = 1 << 2,
    RequiresApplicationRunning   = 1 << 3,
    RequiresSpecificProtocol     = 1 << 4,
    Experimental                 = 1 << 5,
    ReadOnly                     = 1 << 6,
    WriteOnly                    = 1 << 7
};

/**
 * @brief Structured reason for why a constraint exists or an action is unsupported.
 */
enum class ConstraintReason {
    None,
    VendorLimitation,
    FirmwareUnsupported,
    HardwareUnsupported,
    AuthenticationRequired,
    RequiresWakeOnLAN,
    RequiresRunningApplication,
    Experimental,
    Unknown
};

/**
 * @brief Semantic profile of a single supported action.
 */
struct SupportedActionProfile {
    ActionId actionId{ActionId::Unknown};
    SupportState supportState{SupportState::Unknown};
    uint32_t constraints{0};
    ConstraintReason reason{ConstraintReason::None};
};

/**
 * @brief Semantic profile mapping a Capability to its Supported Actions.
 */
struct CapabilityProfile {
    Capability capability;
    std::vector<SupportedActionProfile> supportedActions;
    uint32_t globalConstraints{0};
};

} // namespace NetDiscovery
