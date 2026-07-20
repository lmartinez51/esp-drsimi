#pragma once

#include <string>
#include "parsing/ParsedResponse.h"
#include "transports/soap/SOAPServiceTypes.h"

namespace netdiscovery {
namespace parsing {
namespace soap {

/**
 * @brief Serves as the entry point for parsing SOAP responses.
 * 
 * It routes raw XML to the correct service-specific parser by UPnPService type.
 * It does not instantiate parsers itself; it queries the SOAPParserRegistry.
 */
class SOAPResponseDispatcher {
public:
    /**
     * @brief Parses a raw XML SOAP response body.
     * 
     * @param service The strongly typed UPnP Service enum.
     * @param actionId The strongly typed semantic action (or string ID for now).
     * @param rawXml The raw HTTP payload containing the SOAP envelope.
     * @return ParsedResponse A lightweight model containing the extracted fields and parser diagnostics.
     */
    static ParsedResponse ParseResponse(netdiscovery::soap::UPnPService service, 
                                        const std::string& actionId, 
                                        const std::string& rawXml);
};

} // namespace soap
} // namespace parsing
} // namespace netdiscovery
