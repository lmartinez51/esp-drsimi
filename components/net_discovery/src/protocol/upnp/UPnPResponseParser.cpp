/**
 * @file UPnPResponseParser.cpp
 * @brief Implementation of UPnPResponseParser using tinyxml2 (v5.0.0 Architecture Phase 11).
 */

#include "protocol/upnp/UPnPResponseParser.h"
#include "tinyxml2.h"

#include <cstdlib>

namespace NetDiscovery {
namespace Protocol {
namespace UPnP {

UPnPParsedResponse UPnPResponseParser::Parse(const std::string& xmlBody,
                                             const std::string& expectedActionName) const {
    UPnPParsedResponse response;
    response.actionName = expectedActionName;

    if (xmlBody.empty()) {
        response.parseError = "Empty XML response body";
        return response;
    }

    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError err = doc.Parse(xmlBody.c_str(), xmlBody.size());
    if (err != tinyxml2::XML_SUCCESS) {
        response.parseError = "XML Parse Error: " + std::string(doc.ErrorName());
        return response;
    }

    tinyxml2::XMLElement* root = doc.RootElement();
    if (!root) {
        response.parseError = "Missing XML root element";
        return response;
    }

    // Locate Body element (case insensitive / namespace agnostic search)
    tinyxml2::XMLElement* bodyElem = nullptr;
    for (tinyxml2::XMLElement* elem = root->FirstChildElement(); elem; elem = elem->NextSiblingElement()) {
        std::string name = elem->Name();
        if (name.find("Body") != std::string::npos || name == "Body") {
            bodyElem = elem;
            break;
        }
    }

    if (!bodyElem) {
        // Search root children directly
        bodyElem = root;
    }

    // Check for SOAP Fault
    for (tinyxml2::XMLElement* elem = bodyElem->FirstChildElement(); elem; elem = elem->NextSiblingElement()) {
        std::string elemName = elem->Name();
        if (elemName.find("Fault") != std::string::npos || elemName == "Fault") {
            response.isFault = true;

            tinyxml2::XMLElement* fc = elem->FirstChildElement("faultcode");
            if (!fc) fc = elem->FirstChildElement("faultCode");
            if (fc && fc->GetText()) response.faultCode = fc->GetText();

            tinyxml2::XMLElement* fs = elem->FirstChildElement("faultstring");
            if (!fs) fs = elem->FirstChildElement("faultString");
            if (fs && fs->GetText()) response.faultString = fs->GetText();

            // Extract UPnP UPnPError detail element
            tinyxml2::XMLElement* detail = elem->FirstChildElement("detail");
            if (!detail) detail = elem->FirstChildElement("Detail");

            if (detail) {
                tinyxml2::XMLElement* upnpErr = detail->FirstChildElement("UPnPError");
                if (upnpErr) {
                    tinyxml2::XMLElement* codeElem = upnpErr->FirstChildElement("errorCode");
                    if (codeElem && codeElem->GetText()) {
                        response.upnpErrorCode = std::atoi(codeElem->GetText());
                    }
                    tinyxml2::XMLElement* descElem = upnpErr->FirstChildElement("errorDescription");
                    if (descElem && descElem->GetText()) {
                        response.upnpErrorDescription = descElem->GetText();
                    }
                }
            }
            return response;
        }
    }

    // Extract return parameters from ActionResponse element
    for (tinyxml2::XMLElement* elem = bodyElem->FirstChildElement(); elem; elem = elem->NextSiblingElement()) {
        std::string elemName = elem->Name();
        if (elemName.find("Response") != std::string::npos || !expectedActionName.empty()) {
            for (tinyxml2::XMLElement* param = elem->FirstChildElement(); param; param = param->NextSiblingElement()) {
                std::string pName = param->Name();
                std::string pVal  = param->GetText() ? param->GetText() : "";
                response.returnedValues[pName] = pVal;
            }
            break;
        }
    }

    return response;
}

} // namespace UPnP
} // namespace Protocol
} // namespace NetDiscovery
