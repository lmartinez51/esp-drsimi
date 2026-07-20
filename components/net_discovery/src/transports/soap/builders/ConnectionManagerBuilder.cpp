#include "../../../../include/transports/soap/builders/ConnectionManagerBuilder.h"

namespace NetDiscovery {

SOAPRequest ConnectionManagerBuilder::BuildGetProtocolInfo(const GetProtocolInfoRequest& req)
{
    SOAPRequest soapReq;
    soapReq.endpointUrl = req.controlUrl;
    soapReq.soapAction = "\"urn:schemas-upnp-org:service:ConnectionManager:1#GetProtocolInfo\"";
    soapReq.contentType = "text/xml; charset=\"utf-8\"";
    
    soapReq.xmlBody = 
        "<?xml version=\"1.0\"?>\r\n"
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
        "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
        "  <s:Body>\r\n"
        "    <u:GetProtocolInfo xmlns:u=\"urn:schemas-upnp-org:service:ConnectionManager:1\">\r\n"
        "    </u:GetProtocolInfo>\r\n"
        "  </s:Body>\r\n"
        "</s:Envelope>\r\n";
        
    return soapReq;
}

} // namespace NetDiscovery
