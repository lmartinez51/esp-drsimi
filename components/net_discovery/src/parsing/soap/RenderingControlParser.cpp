#include "parsing/soap/RenderingControlParser.h"

namespace netdiscovery {
namespace parsing {
namespace soap {

ParsedResponse RenderingControlParser::Parse(const std::string& actionId, const tinyxml2::XMLDocument& xmlDoc) {
    ParsedResponse response;
    response.success = false;

    // Navigate to Envelope -> Body -> {Action}Response
    const tinyxml2::XMLElement* envelope = xmlDoc.FirstChildElement("s:Envelope");
    if (!envelope) envelope = xmlDoc.FirstChildElement("Envelope"); // fallback for missing namespace prefix
    
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
        // Some devices don't namespace the response element correctly
        actionResponse = body->FirstChildElement((actionId + "Response").c_str());
    }

    if (!actionResponse) {
        response.diagnostics.hasError = true;
        response.diagnostics.errorMessage = "Missing " + actionId + "Response element";
        return response;
    }

    // Extract fields based on actionId
    if (actionId == "GetVolume") {
        const tinyxml2::XMLElement* vol = actionResponse->FirstChildElement("CurrentVolume");
        if (vol && vol->GetText()) {
            response.fields["CurrentVolume"] = vol->GetText();
            response.success = true;
        } else {
            response.diagnostics.hasError = true;
            response.diagnostics.errorMessage = "Missing CurrentVolume node";
        }
    } 
    else if (actionId == "GetMute") {
        const tinyxml2::XMLElement* mute = actionResponse->FirstChildElement("CurrentMute");
        if (mute && mute->GetText()) {
            response.fields["CurrentMute"] = mute->GetText();
            response.success = true;
        } else {
            response.diagnostics.hasError = true;
            response.diagnostics.errorMessage = "Missing CurrentMute node";
        }
    }
    else {
        response.diagnostics.hasError = true;
        response.diagnostics.errorMessage = "Unsupported action for RenderingControlParser: " + actionId;
    }

    return response;
}

} // namespace soap
} // namespace parsing
} // namespace netdiscovery
