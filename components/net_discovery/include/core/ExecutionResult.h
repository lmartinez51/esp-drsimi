/**
 * @file ExecutionResult.h
 * @brief Represents the outcome of an execution request.
 */

#pragma once

#include <string>

#include "core/TransportDiagnostics.h"
#include "parsing/ParsedResponse.h"

namespace NetDiscovery {

enum class ExecutionStatus {
    Success,
    UnsupportedAction,
    TransportUnavailable,
    Timeout,
    AuthenticationRequired,
    ProtocolError,
    ParseError,
    ExecutionFailed
};

inline std::string ToString(ExecutionStatus status) {
    switch (status) {
        case ExecutionStatus::Success:                return "Success";
        case ExecutionStatus::UnsupportedAction:      return "UnsupportedAction";
        case ExecutionStatus::TransportUnavailable:   return "TransportUnavailable";
        case ExecutionStatus::Timeout:                return "Timeout";
        case ExecutionStatus::AuthenticationRequired: return "AuthenticationRequired";
        case ExecutionStatus::ProtocolError:          return "ProtocolError";
        case ExecutionStatus::ParseError:             return "ParseError";
        case ExecutionStatus::ExecutionFailed:        return "ExecutionFailed";
        default:                                      return "Unknown";
    }
}

/**
 * @brief Structured result from an ICommunicationTransport execution.
 */
struct ExecutionResult {
    ExecutionStatus status;
    int elapsedTimeMs{0};
    std::string errorMessage;
    
    // Transport layer diagnostics
    TransportDiagnostics transportDiagnostics;
    
    // Parsed protocol fields (populated by the Execution Engine invoking a parser)
    std::unordered_map<std::string, std::string> parsedFields;
    
    // Parser layer diagnostics
    netdiscovery::parsing::ParserDiagnostics parserDiagnostics;
};

} // namespace NetDiscovery
