#include <iostream>
#include "parsing/soap/SOAPResponseDispatcher.h"
#include "transports/soap/SOAPServiceTypes.h"
#include "parsing/soap/SOAPParserRegistry.h"
#include "parsing/soap/RenderingControlParser.h"

using namespace netdiscovery::parsing::soap;
using namespace netdiscovery::soap;

int main() {
    SOAPParserRegistry::GetInstance(); // Initializes parsers

    std::string xml = R"(<?xml version="1.0"?>
<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
  <s:Body>
    <u:GetVolumeResponse xmlns:u="urn:schemas-upnp-org:service:RenderingControl:1">
      <CurrentVolume>15</CurrentVolume>
    </u:GetVolumeResponse>
  </s:Body>
</s:Envelope>)";

    auto res = SOAPResponseDispatcher::ParseResponse(UPnPService::RenderingControl, "GetVolume", xml);
    
    if (res.success) {
        std::cout << "SUCCESS!\n";
        for (const auto& [k, v] : res.fields) {
            std::cout << k << " = " << v << "\n";
        }
    } else {
        std::cout << "FAILED: " << res.diagnostics.errorMessage << "\n";
    }
    
    return 0;
}
