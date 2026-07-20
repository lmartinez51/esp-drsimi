#pragma once

#include <string>
#include "parsing/ParsedResponse.h"
#include "core/ActionDescriptor.h"
#include "tinyxml2.h"

namespace netdiscovery {
namespace parsing {
namespace soap {

/**
 * @brief Interface for parsing responses from a specific UPnP SOAP service.
 * 
 * Each concrete parser (e.g. RenderingControlParser) implements this interface
 * to extract fields from the parsed TinyXML2 document, based on the semantic action requested.
 */
class ISOAPServiceParser {
public:
    virtual ~ISOAPServiceParser() = default;

    /**
     * @brief Extract data from a successful SOAP response body.
     * 
     * @param actionId The semantic action that generated this response (e.g. "GetVolume").
     * @param xmlDoc The already-parsed TinyXML2 document containing the SOAP Envelope.
     * @return ParsedResponse The extracted fields and diagnostics.
     */
    virtual ParsedResponse Parse(const std::string& actionId, const tinyxml2::XMLDocument& xmlDoc) = 0;
};

} // namespace soap
} // namespace parsing
} // namespace netdiscovery
