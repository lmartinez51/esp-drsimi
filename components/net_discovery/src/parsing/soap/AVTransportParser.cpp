#include "parsing/soap/AVTransportParser.h"

namespace netdiscovery {
namespace parsing {
namespace soap {

ParsedResponse AVTransportParser::Parse(const std::string& actionId, const tinyxml2::XMLDocument& xmlDoc) {
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

    if (actionId == "GetTransportInfo") {
        const tinyxml2::XMLElement* state = actionResponse->FirstChildElement("CurrentTransportState");
        const tinyxml2::XMLElement* status = actionResponse->FirstChildElement("CurrentTransportStatus");
        const tinyxml2::XMLElement* speed = actionResponse->FirstChildElement("CurrentSpeed");

        if (state && state->GetText()) {
            response.fields["CurrentTransportState"] = state->GetText();
            response.success = true; // at least one field found
        }
        if (status && status->GetText()) {
            response.fields["CurrentTransportStatus"] = status->GetText();
        }
        if (speed && speed->GetText()) {
            response.fields["CurrentSpeed"] = speed->GetText();
        }
    }
    else if (actionId == "GetPositionInfo") {
        const tinyxml2::XMLElement* track = actionResponse->FirstChildElement("Track");
        const tinyxml2::XMLElement* relTime = actionResponse->FirstChildElement("RelTime");
        const tinyxml2::XMLElement* absTime = actionResponse->FirstChildElement("AbsTime");

        if (track && track->GetText()) response.fields["Track"] = track->GetText();
        if (relTime && relTime->GetText()) response.fields["RelTime"] = relTime->GetText();
        if (absTime && absTime->GetText()) response.fields["AbsTime"] = absTime->GetText();
        
        response.success = true;
    }
    else {
        response.diagnostics.hasError = true;
        response.diagnostics.errorMessage = "Unsupported action for AVTransportParser: " + actionId;
    }

    return response;
}

} // namespace soap
} // namespace parsing
} // namespace netdiscovery
