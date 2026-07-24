/**
 * @file ExecutionEvents.h
 * @brief Event metadata constants for StorageEventBus (v5.0.0 Architecture Phase 8.5).
 */

#pragma once

#include "core/StorageEventType.h"

namespace NetDiscovery {
namespace Execution {

struct ExecutionEventKeys {
    static constexpr const char* PlanId = "PlanId";
    static constexpr const char* RequestId = "RequestId";
    static constexpr const char* TargetEntityId = "TargetEntityId";
    static constexpr const char* StepCount = "StepCount";
    static constexpr const char* EstimatedDurationMs = "EstimatedDurationMs";
    static constexpr const char* PolicyMode = "PolicyMode";
    static constexpr const char* ValidationFailureReason = "ValidationFailureReason";
};

} // namespace Execution
} // namespace NetDiscovery
