/**
 * @file ExecutionEvent.h
 * @brief Metadata keys for runtime execution domain events (v5.0.0 Architecture Phase 9).
 */

#pragma once

#include <string>

namespace NetDiscovery {
namespace Execution {

namespace RuntimeEventKeys {
    constexpr const char* SessionId = "SessionId";
    constexpr const char* PlanId = "PlanId";
    constexpr const char* RequestId = "RequestId";
    constexpr const char* State = "State";
    constexpr const char* StepId = "StepId";
    constexpr const char* AdapterId = "AdapterId";
    constexpr const char* StepStatus = "StepStatus";
    constexpr const char* ErrorMessage = "ErrorMessage";
    constexpr const char* DurationMs = "DurationMs";
} // namespace RuntimeEventKeys

} // namespace Execution
} // namespace NetDiscovery
