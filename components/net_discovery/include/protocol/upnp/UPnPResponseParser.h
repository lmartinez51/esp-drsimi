/**
 * @file UPnPResponseParser.h
 * @brief Pure parser component extracting response values or SOAP faults from XML (v5.0.0 Architecture Phase 11).
 */

#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>

namespace NetDiscovery {
namespace Protocol {
namespace UPnP {

/**
 * @brief Structured outcome extracted by UPnPResponseParser.
 */
struct UPnPParsedResponse {
    bool        isFault{false};
    std::string actionName;
    std::unordered_map<std::string, std::string> returnedValues;
    std::string faultCode;
    std::string faultString;
    int32_t     upnpErrorCode{0};
    std::string upnpErrorDescription;
    std::string parseError;

    bool IsSuccess() const { return !isFault && parseError.empty(); }
};

/**
 * @brief Pure XML parser for UPnP SOAP response bodies.
 *
 * Uses tinyxml2 to parse XML envelopes. Performs NO network operations or execution.
 */
class UPnPResponseParser {
public:
    UPnPResponseParser() = default;

    /**
     * @brief Parses an HTTP response body into a UPnPParsedResponse structure.
     */
    UPnPParsedResponse Parse(const std::string& xmlBody,
                             const std::string& expectedActionName = "") const;
};

} // namespace UPnP
} // namespace Protocol
} // namespace NetDiscovery
