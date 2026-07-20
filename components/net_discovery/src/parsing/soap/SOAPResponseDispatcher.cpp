#include "parsing/soap/SOAPResponseDispatcher.h"
#include "parsing/soap/SOAPParserRegistry.h"
#include "tinyxml2.h"

namespace netdiscovery {
namespace parsing {
namespace soap {

ParsedResponse SOAPResponseDispatcher::ParseResponse(netdiscovery::soap::UPnPService service, 
                                                     const std::string& actionId, 
                                                     const std::string& rawXml) 
{
    ParsedResponse response;
    
    if (rawXml.empty()) {
        response.success = false;
        response.diagnostics.hasError = true;
        response.diagnostics.errorMessage = "Empty XML payload";
        return response;
    }

    auto parser = SOAPParserRegistry::GetInstance().GetParser(service);
    if (!parser) {
        response.success = false;
        response.diagnostics.hasError = true;
        response.diagnostics.errorMessage = "No parser registered for requested UPnPService";
        return response;
    }

    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError err = doc.Parse(rawXml.c_str());
    if (err != tinyxml2::XML_SUCCESS) {
        response.success = false;
        response.diagnostics.hasError = true;
        response.diagnostics.errorMessage = "TinyXML2 failed to parse document: " + std::string(doc.ErrorStr());
        return response;
    }

    // Hand off the parsed DOM to the specific service parser
    return parser->Parse(actionId, doc);
}

} // namespace soap
} // namespace parsing
} // namespace netdiscovery
