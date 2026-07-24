/**
 * @file HTTPResponseMapper.cpp
 * @brief Implementation of HTTPResponseMapper (v5.0.0 Architecture Phase 15).
 */

#include "protocol/http/HTTPResponseMapper.h"

namespace NetDiscovery {
namespace Protocol {
namespace HTTP {

Runtime::ExecutionStepResult HTTPResponseMapper::MapToStepResult(
        const std::string&         stepId,
        const std::string&         adapterId,
        const HTTPResponse&        response,
        const HTTPParsedResponse&  parsed) const {

    // 1. Success path (2xx HTTP status codes)
    if (response.IsSuccess()) {
        return Runtime::ExecutionStepResult(
            stepId,
            Execution::StepStatus::Success,
            adapterId,
            response.latencyMs,
            response.latencyMs,
            0,
            "",
            false, false,
            {"httpStatus=" + std::to_string(response.statusCode)},
            parsed.parsedFields,
            {{"protocol", "HTTP"}, {"statusCode", std::to_string(response.statusCode)}});
    }

    // 2. Timeout (-101)
    if (response.IsTimeout()) {
        return Runtime::ExecutionStepResult(
            stepId,
            Execution::StepStatus::Timeout,
            adapterId,
            response.latencyMs,
            response.latencyMs,
            -101,
            "HTTP Timeout (" + response.statusText + ")",
            true, false,
            {"timeoutMs=" + std::to_string(response.latencyMs)},
            {},
            {{"protocol", "HTTP"}, {"errorType", "Timeout"}},
            Runtime::ExecutionFailureReason::Timeout,
            "HTTPAdapter",
            -101,
            response.statusText);
    }

    // 3. Client Error (4xx HTTP status codes)
    if (response.IsClientError()) {
        Runtime::ExecutionFailureReason reason = Runtime::ExecutionFailureReason::ProtocolFailure;
        if (response.statusCode == 401 || response.statusCode == 403) {
            reason = Runtime::ExecutionFailureReason::AuthenticationFailure;
        } else if (response.statusCode == 404) {
            reason = Runtime::ExecutionFailureReason::DeviceUnavailable;
        }

        return Runtime::ExecutionStepResult(
            stepId,
            Execution::StepStatus::Failure,
            adapterId,
            response.latencyMs,
            response.latencyMs,
            response.statusCode,
            "HTTP Client Error (" + std::to_string(response.statusCode) + "): " + response.statusText,
            false, false,
            {"statusCode=" + std::to_string(response.statusCode)},
            {},
            {{"protocol", "HTTP"}, {"errorType", "ClientError"}},
            reason,
            "HTTPAdapter",
            response.statusCode,
            response.statusText);
    }

    // 4. Server Error (5xx HTTP status codes)
    bool retrySuggested = (response.statusCode == 502 || response.statusCode == 503 || response.statusCode == 504);

    return Runtime::ExecutionStepResult(
        stepId,
        Execution::StepStatus::Failure,
        adapterId,
        response.latencyMs,
        response.latencyMs,
        response.statusCode,
        "HTTP Server Error (" + std::to_string(response.statusCode) + "): " + response.statusText,
        retrySuggested, false,
        {"statusCode=" + std::to_string(response.statusCode)},
        {},
        {{"protocol", "HTTP"}, {"errorType", "ServerError"}},
        Runtime::ExecutionFailureReason::TransportFailure,
        "HTTPAdapter",
        response.statusCode,
        response.statusText);
}

} // namespace HTTP
} // namespace Protocol
} // namespace NetDiscovery
