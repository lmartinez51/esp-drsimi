/**
 * @file ExecutionStepResult.h
 * @brief Outcome representation of an individual ExecutionStep execution (v5.0.0 Architecture Phase 9).
 */

#pragma once

#include "execution/ExecutionPlannerTypes.h"

#include <string>
#include <unordered_map>
#include <cstdint>

namespace NetDiscovery {
namespace Execution {

enum class StepStatus {
    Success,
    Failure,
    Retry,
    Timeout,
    Cancelled,
    Skipped,
    Deferred,
    NotImplemented,
    RollbackRequired
};

inline std::string ToString(StepStatus status) {
    switch (status) {
        case StepStatus::Success:          return "Success";
        case StepStatus::Failure:          return "Failure";
        case StepStatus::Retry:            return "Retry";
        case StepStatus::Timeout:          return "Timeout";
        case StepStatus::Cancelled:        return "Cancelled";
        case StepStatus::Skipped:          return "Skipped";
        case StepStatus::Deferred:         return "Deferred";
        case StepStatus::NotImplemented:   return "NotImplemented";
        case StepStatus::RollbackRequired: return "RollbackRequired";
        default:                           return "Unknown";
    }
}

/**
 * @brief Value object capturing result outcome metrics of an individual step.
 */
struct ExecutionStepResult {
    StepStatus status{StepStatus::NotImplemented};
    StepId stepId;
    uint32_t durationMs{0};
    std::string errorMessage;
    std::unordered_map<std::string, std::string> outputData;
    std::unordered_map<std::string, std::string> metadata;

    ExecutionStepResult() = default;

    ExecutionStepResult(StepStatus stat, StepId sId, uint32_t dur = 0, std::string err = "",
                        std::unordered_map<std::string, std::string> out = {},
                        std::unordered_map<std::string, std::string> meta = {})
        : status(stat), stepId(std::move(sId)), durationMs(dur), errorMessage(std::move(err)),
          outputData(std::move(out)), metadata(std::move(meta)) {}

    bool IsSuccess() const { return status == StepStatus::Success; }
    bool IsFatal() const { return status == StepStatus::Failure || status == StepStatus::RollbackRequired; }
};

} // namespace Execution
} // namespace NetDiscovery
