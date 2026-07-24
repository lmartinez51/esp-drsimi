/**
 * @file UPnPErrorMapper.cpp
 * @brief Implementation of UPnPErrorMapper (v5.0.0 Architecture Phase 11.1).
 */

#include "protocol/upnp/UPnPErrorMapper.h"

namespace NetDiscovery {
namespace Protocol {
namespace UPnP {

Runtime::ExecutionStepResult UPnPErrorMapper::MapToStepResult(
        const std::string&         stepId,
        const std::string&         adapterId,
        const UPnPResponse&        response,
        const UPnPParsedResponse&  parsed) const {

    // 1. Success path
    if (response.IsSuccess() && parsed.IsSuccess()) {
        return Runtime::ExecutionStepResult(
            stepId,
            Execution::StepStatus::Success,
            adapterId,
            response.elapsedTimeMs,
            response.elapsedTimeMs,
            0,
            "",
            false, false,
            {"httpStatus=200", "elapsedTimeMs=" + std::to_string(response.elapsedTimeMs)},
            parsed.returnedValues,
            {{"protocol", "UPnP"}, {"transport", "HTTP/SOAP"}});
    }

    // 2. HTTP Transport Error (e.g. Timeout / 408)
    if (response.IsTimeout()) {
        return Runtime::ExecutionStepResult(
            stepId,
            Execution::StepStatus::Timeout,
            adapterId,
            response.elapsedTimeMs,
            response.elapsedTimeMs,
            -101,
            "UPnP HTTP Transport Timeout (" + response.statusText + ")",
            true, false,
            {"timeoutMs=" + std::to_string(response.elapsedTimeMs)},
            {},
            {{"protocol", "UPnP"}, {"errorType", "Timeout"}});
    }

    // 3. XML Parse Error
    if (!parsed.parseError.empty()) {
        return Runtime::ExecutionStepResult(
            stepId,
            Execution::StepStatus::Failure,
            adapterId,
            response.elapsedTimeMs,
            response.elapsedTimeMs,
            -102,
            "UPnP XML Parse Error: " + parsed.parseError,
            false, false,
            {parsed.parseError},
            {},
            {{"protocol", "UPnP"}, {"errorType", "ParseError"}});
    }

    // 4. SOAP Fault Error Mapping
    if (parsed.isFault) {
        Execution::StepStatus status = Execution::StepStatus::Failure;
        bool retrySuggested = false;
        std::string userMsg;

        switch (parsed.upnpErrorCode) {
            case 401: userMsg = "Invalid Action"; status = Execution::StepStatus::NotImplemented; break;
            case 402: userMsg = "Invalid Parameter Arguments"; break;
            case 501: userMsg = "Action Failed on UPnP Device"; retrySuggested = true; break;
            case 701: userMsg = "Object / Service Not Found"; break;
            case 702: userMsg = "Device Busy"; status = Execution::StepStatus::Retry; retrySuggested = true; break;
            default:
                userMsg = !parsed.upnpErrorDescription.empty() ? parsed.upnpErrorDescription :
                         (!parsed.faultString.empty() ? parsed.faultString : "UPnP SOAP Fault " + std::to_string(parsed.upnpErrorCode));
                break;
        }

        return Runtime::ExecutionStepResult(
            stepId,
            status,
            adapterId,
            response.elapsedTimeMs,
            response.elapsedTimeMs,
            parsed.upnpErrorCode != 0 ? parsed.upnpErrorCode : -103,
            "UPnP Protocol Error: " + userMsg,
            retrySuggested, false,
            {"soapFaultCode=" + parsed.faultCode, "upnpErrorCode=" + std::to_string(parsed.upnpErrorCode)},
            {},
            {{"protocol", "UPnP"}, {"errorType", "SOAPFault"}});
    }

    // 5. Generic HTTP Error fallback
    return Runtime::ExecutionStepResult(
        stepId,
        Execution::StepStatus::Failure,
        adapterId,
        response.elapsedTimeMs,
        response.elapsedTimeMs,
        response.statusCode,
        "HTTP " + std::to_string(response.statusCode) + ": " + response.statusText,
        (response.statusCode >= 500), false,
        {"httpStatusCode=" + std::to_string(response.statusCode)},
        {},
        {{"protocol", "UPnP"}, {"errorType", "HTTPError"}});
}

} // namespace UPnP
} // namespace Protocol
} // namespace NetDiscovery
