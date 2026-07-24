/**
 * @file ExecutionStepResult.h
 * @brief Complete outcome model returned by IExecutionDispatcher (v5.0.0 Architecture Phase 13.1).
 */

#pragma once

#include "execution/ExecutionPlannerTypes.h"
#include "execution/ExecutionStepResult.h"
#include "runtime/ExecutionFailureReason.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <utility>

namespace NetDiscovery {
namespace Runtime {

/**
 * @brief Structured outcome returned by protocol adapter dispatchers.
 */
struct ExecutionStepResult {
    Execution::StepId stepId;
    Execution::StepStatus status{Execution::StepStatus::NotImplemented};
    ExecutionFailureReason failureReason{ExecutionFailureReason::None};
    std::string failureSource;
    int32_t     diagnosticCode{0};
    std::string diagnosticMessage;
    std::string adapterId;
    uint32_t executionDurationMs{0};
    uint32_t protocolLatencyMs{0};
    int32_t errorCode{0};
    std::string errorMessage;
    bool retrySuggested{false};
    bool rollbackRequired{false};
    std::vector<std::string> diagnostics;
    std::unordered_map<std::string, std::string> returnedValues;
    std::unordered_map<std::string, std::string> metadata;

    ExecutionStepResult() = default;

    ExecutionStepResult(Execution::StepId sId, Execution::StepStatus stat, std::string aId = "",
                        uint32_t execDurMs = 0, uint32_t protLatMs = 0, int32_t errCode = 0,
                        std::string errMsg = "", bool retry = false, bool rollback = false,
                        std::vector<std::string> diag = {},
                        std::unordered_map<std::string, std::string> retVals = {},
                        std::unordered_map<std::string, std::string> meta = {},
                        ExecutionFailureReason failReason = ExecutionFailureReason::None,
                        std::string failSource = "",
                        int32_t diagCode = 0,
                        std::string diagMsg = "")
        : stepId(std::move(sId)), status(stat), failureReason(failReason),
          failureSource(std::move(failSource)), diagnosticCode(diagCode),
          diagnosticMessage(std::move(diagMsg)), adapterId(std::move(aId)),
          executionDurationMs(execDurMs), protocolLatencyMs(protLatMs), errorCode(errCode),
          errorMessage(std::move(errMsg)), retrySuggested(retry), rollbackRequired(rollback),
          diagnostics(std::move(diag)), returnedValues(std::move(retVals)), metadata(std::move(meta)) {}

    bool IsSuccess() const { return status == Execution::StepStatus::Success; }
    bool IsFatal() const { return status == Execution::StepStatus::Failure || rollbackRequired; }

    ExecutionFailureReason GetFailureReason() const { return failureReason; }
    const std::string& GetFailureSource() const { return failureSource; }
    int32_t GetDiagnosticCode() const { return diagnosticCode; }
    const std::string& GetDiagnosticMessage() const { return diagnosticMessage; }
};

} // namespace Runtime
} // namespace NetDiscovery
