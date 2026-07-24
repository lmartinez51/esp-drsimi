/**
 * @file ExecutionEvent.h
 * @brief Runtime event types and structure for execution event queueing (v5.0.0 Architecture Phase 9.1).
 */

#pragma once

#include "execution/ExecutionPlannerTypes.h"

#include <string>
#include <unordered_map>
#include <cstdint>
#include <utility>

namespace NetDiscovery {
namespace Runtime {

/**
 * @brief Strongly typed runtime event classifications.
 */
enum class ExecutionEventType {
    StepStarted,
    StepCompleted,
    StepFailed,
    Timeout,
    RetryRequested,
    PauseRequested,
    ResumeRequested,
    CancelRequested,
    ExternalSignal,
    AuthenticationExpired
};

inline std::string ToString(ExecutionEventType type) {
    switch (type) {
        case ExecutionEventType::StepStarted:           return "StepStarted";
        case ExecutionEventType::StepCompleted:         return "StepCompleted";
        case ExecutionEventType::StepFailed:            return "StepFailed";
        case ExecutionEventType::Timeout:               return "Timeout";
        case ExecutionEventType::RetryRequested:        return "RetryRequested";
        case ExecutionEventType::PauseRequested:        return "PauseRequested";
        case ExecutionEventType::ResumeRequested:       return "ResumeRequested";
        case ExecutionEventType::CancelRequested:       return "CancelRequested";
        case ExecutionEventType::ExternalSignal:        return "ExternalSignal";
        case ExecutionEventType::AuthenticationExpired: return "AuthenticationExpired";
        default:                                        return "Unknown";
    }
}

/**
 * @brief Runtime event value object pushed into ExecutionEventQueue.
 */
struct ExecutionEvent {
    ExecutionEventType type{ExecutionEventType::StepStarted};
    Execution::StepId stepId;
    uint64_t timestampMs{0};
    std::string source;
    std::unordered_map<std::string, std::string> payload;

    ExecutionEvent() = default;

    ExecutionEvent(ExecutionEventType t, Execution::StepId sId = "", uint64_t tsMs = 0, std::string src = "",
                   std::unordered_map<std::string, std::string> pl = {})
        : type(t), stepId(std::move(sId)), timestampMs(tsMs), source(std::move(src)), payload(std::move(pl)) {}
};

} // namespace Runtime
} // namespace NetDiscovery
