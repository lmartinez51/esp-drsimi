#include "../../../../include/transports/soap/builders/AVTransportBuilder.h"

namespace NetDiscovery {

SOAPRequest AVTransportBuilder::BuildPlay(const PlayRequest& req) {
    SOAPRequest soapReq;
    soapReq.endpointUrl = req.controlUrl;
    soapReq.soapAction = "\"urn:schemas-upnp-org:service:AVTransport:1#Play\"";
    soapReq.contentType = "text/xml; charset=\"utf-8\"";
    soapReq.timeoutMs = 5000;
    soapReq.extraHeaders = { {"User-Agent", "NetDiscovery/1.0"}, {"Connection", "close"} };
    
    soapReq.xmlBody = 
        "<?xml version=\"1.0\"?>\r\n"
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
        "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
        "  <s:Body>\r\n"
        "    <u:Play xmlns:u=\"urn:schemas-upnp-org:service:AVTransport:1\">\r\n"
        "      <InstanceID>" + std::to_string(req.instanceID) + "</InstanceID>\r\n"
        "      <Speed>" + req.speed + "</Speed>\r\n"
        "    </u:Play>\r\n"
        "  </s:Body>\r\n"
        "</s:Envelope>\r\n";
    return soapReq;
}

SOAPRequest AVTransportBuilder::BuildPause(const PauseRequest& req) {
    SOAPRequest soapReq;
    soapReq.endpointUrl = req.controlUrl;
    soapReq.soapAction = "\"urn:schemas-upnp-org:service:AVTransport:1#Pause\"";
    soapReq.contentType = "text/xml; charset=\"utf-8\"";
    soapReq.timeoutMs = 5000;
    soapReq.extraHeaders = { {"User-Agent", "NetDiscovery/1.0"}, {"Connection", "close"} };
    
    soapReq.xmlBody = 
        "<?xml version=\"1.0\"?>\r\n"
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
        "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
        "  <s:Body>\r\n"
        "    <u:Pause xmlns:u=\"urn:schemas-upnp-org:service:AVTransport:1\">\r\n"
        "      <InstanceID>" + std::to_string(req.instanceID) + "</InstanceID>\r\n"
        "    </u:Pause>\r\n"
        "  </s:Body>\r\n"
        "</s:Envelope>\r\n";
    return soapReq;
}

SOAPRequest AVTransportBuilder::BuildStop(const StopRequest& req) {
    SOAPRequest soapReq;
    soapReq.endpointUrl = req.controlUrl;
    soapReq.soapAction = "\"urn:schemas-upnp-org:service:AVTransport:1#Stop\"";
    soapReq.contentType = "text/xml; charset=\"utf-8\"";
    soapReq.timeoutMs = 5000;
    soapReq.extraHeaders = { {"User-Agent", "NetDiscovery/1.0"}, {"Connection", "close"} };
    
    soapReq.xmlBody = 
        "<?xml version=\"1.0\"?>\r\n"
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
        "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
        "  <s:Body>\r\n"
        "    <u:Stop xmlns:u=\"urn:schemas-upnp-org:service:AVTransport:1\">\r\n"
        "      <InstanceID>" + std::to_string(req.instanceID) + "</InstanceID>\r\n"
        "    </u:Stop>\r\n"
        "  </s:Body>\r\n"
        "</s:Envelope>\r\n";
    return soapReq;
}

SOAPRequest AVTransportBuilder::BuildNext(const NextRequest& req) {
    SOAPRequest soapReq;
    soapReq.endpointUrl = req.controlUrl;
    soapReq.soapAction = "\"urn:schemas-upnp-org:service:AVTransport:1#Next\"";
    soapReq.contentType = "text/xml; charset=\"utf-8\"";
    soapReq.timeoutMs = 5000;
    soapReq.extraHeaders = { {"User-Agent", "NetDiscovery/1.0"}, {"Connection", "close"} };
    
    soapReq.xmlBody = 
        "<?xml version=\"1.0\"?>\r\n"
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
        "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
        "  <s:Body>\r\n"
        "    <u:Next xmlns:u=\"urn:schemas-upnp-org:service:AVTransport:1\">\r\n"
        "      <InstanceID>" + std::to_string(req.instanceID) + "</InstanceID>\r\n"
        "    </u:Next>\r\n"
        "  </s:Body>\r\n"
        "</s:Envelope>\r\n";
    return soapReq;
}

SOAPRequest AVTransportBuilder::BuildPrevious(const PreviousRequest& req) {
    SOAPRequest soapReq;
    soapReq.endpointUrl = req.controlUrl;
    soapReq.soapAction = "\"urn:schemas-upnp-org:service:AVTransport:1#Previous\"";
    soapReq.contentType = "text/xml; charset=\"utf-8\"";
    soapReq.timeoutMs = 5000;
    soapReq.extraHeaders = { {"User-Agent", "NetDiscovery/1.0"}, {"Connection", "close"} };
    
    soapReq.xmlBody = 
        "<?xml version=\"1.0\"?>\r\n"
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
        "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
        "  <s:Body>\r\n"
        "    <u:Previous xmlns:u=\"urn:schemas-upnp-org:service:AVTransport:1\">\r\n"
        "      <InstanceID>" + std::to_string(req.instanceID) + "</InstanceID>\r\n"
        "    </u:Previous>\r\n"
        "  </s:Body>\r\n"
        "</s:Envelope>\r\n";
    return soapReq;
}

SOAPRequest AVTransportBuilder::BuildSeek(const SeekRequest& req) {
    SOAPRequest soapReq;
    soapReq.endpointUrl = req.controlUrl;
    soapReq.soapAction = "\"urn:schemas-upnp-org:service:AVTransport:1#Seek\"";
    soapReq.contentType = "text/xml; charset=\"utf-8\"";
    soapReq.timeoutMs = 5000;
    soapReq.extraHeaders = { {"User-Agent", "NetDiscovery/1.0"}, {"Connection", "close"} };
    
    soapReq.xmlBody = 
        "<?xml version=\"1.0\"?>\r\n"
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
        "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
        "  <s:Body>\r\n"
        "    <u:Seek xmlns:u=\"urn:schemas-upnp-org:service:AVTransport:1\">\r\n"
        "      <InstanceID>" + std::to_string(req.instanceID) + "</InstanceID>\r\n"
        "      <Unit>" + req.unit + "</Unit>\r\n"
        "      <Target>" + req.target + "</Target>\r\n"
        "    </u:Seek>\r\n"
        "  </s:Body>\r\n"
        "</s:Envelope>\r\n";
    return soapReq;
}

} // namespace NetDiscovery