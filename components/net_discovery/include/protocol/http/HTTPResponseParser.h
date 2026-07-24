/**
 * @file HTTPResponseParser.h
 * @brief Pure HTTP response payload parser (v5.0.0 Architecture Phase 15).
 */

#pragma once

#include <string>
#include <unordered_map>

namespace NetDiscovery {
namespace Protocol {
namespace HTTP {

/**
 * @brief Parsed HTTP response structure.
 */
struct HTTPParsedResponse {
    std::string contentType{"application/json"};
    bool        isSuccessParse{true};
    std::string parseError;
    std::unordered_map<std::string, std::string> parsedFields;

    HTTPParsedResponse() = default;
};

/**
 * @brief Pure payload parser for JSON, text/plain, and binary HTTP payloads.
 *
 * Contains ZERO transport or socket logic.
 */
class HTTPResponseParser {
public:
    HTTPResponseParser() = default;

    /**
     * @brief Parses an HTTP payload body based on contentType.
     */
    HTTPParsedResponse Parse(const std::string& payload,
                             const std::string& contentType = "application/json") const;
};

} // namespace HTTP
} // namespace Protocol
} // namespace NetDiscovery
