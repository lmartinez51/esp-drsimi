#include "parsing/soap/ConnectionManagerParser.h"

namespace netdiscovery {
namespace parsing {
namespace soap {

ParsedResponse ConnectionManagerParser::Parse(const std::string& actionId, const tinyxml2::XMLDocument& xmlDoc) {
    ParsedResponse response;
    response.success = false;

    const tinyxml2::XMLElement* envelope = xmlDoc.FirstChildElement("s:Envelope");
    if (!envelope) envelope = xmlDoc.FirstChildElement("Envelope");
    
    if (!envelope) {
        response.diagnostics.hasError = true;
        response.diagnostics.errorMessage = "Missing SOAP Envelope";
        return response;
    }

    const tinyxml2::XMLElement* body = envelope->FirstChildElement("s:Body");
    if (!body) body = envelope->FirstChildElement("Body");
    
    if (!body) {
        response.diagnostics.hasError = true;
        response.diagnostics.errorMessage = "Missing SOAP Body";
        return response;
    }

    std::string responseTagName = "u:" + actionId + "Response";
    const tinyxml2::XMLElement* actionResponse = body->FirstChildElement(responseTagName.c_str());
    if (!actionResponse) {
        actionResponse = body->FirstChildElement((actionId + "Response").c_str());
    }

    if (!actionResponse) {
        response.diagnostics.hasError = true;
        response.diagnostics.errorMessage = "Missing " + actionId + "Response element";
        return response;
    }

    if (actionId == "GetProtocolInfo") {
        const tinyxml2::XMLElement* source = actionResponse->FirstChildElement("Source");
        const tinyxml2::XMLElement* sink = actionResponse->FirstChildElement("Sink");

        if (source && source->GetText()) response.fields["Source"] = source->GetText();
        if (sink && sink->GetText()) response.fields["Sink"] = sink->GetText();
        
        response.success = true;
    }
    else {
        response.diagnostics.hasError = true;
        response.diagnostics.errorMessage = "Unsupported action for ConnectionManagerParser: " + actionId;
    }

    return response;
}

} // namespace soap
} // namespace parsing
} // namespace netdiscovery
