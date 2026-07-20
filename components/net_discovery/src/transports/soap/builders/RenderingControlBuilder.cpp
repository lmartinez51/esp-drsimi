#include "../../../../include/transports/soap/builders/RenderingControlBuilder.h"

namespace NetDiscovery {

SOAPRequest RenderingControlBuilder::BuildGetVolume(const GetVolumeRequest& req) {
    SOAPRequest soapReq;
    soapReq.endpointUrl = req.controlUrl;
    soapReq.soapAction = "\"urn:schemas-upnp-org:service:RenderingControl:1#GetVolume\"";
    soapReq.contentType = "text/xml; charset=\"utf-8\"";
    soapReq.timeoutMs = 5000;
    soapReq.extraHeaders = { {"User-Agent", "NetDiscovery/1.0"}, {"Connection", "close"} };
    
    soapReq.xmlBody = 
        "<?xml version=\"1.0\"?>\r\n"
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
        "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
        "  <s:Body>\r\n"
        "    <u:GetVolume xmlns:u=\"urn:schemas-upnp-org:service:RenderingControl:1\">\r\n"
        "      <InstanceID>" + std::to_string(req.instanceID) + "</InstanceID>\r\n"
        "      <Channel>" + req.channel + "</Channel>\r\n"
        "    </u:GetVolume>\r\n"
        "  </s:Body>\r\n"
        "</s:Envelope>\r\n";
    return soapReq;
}

SOAPRequest RenderingControlBuilder::BuildSetVolume(const SetVolumeRequest& req) {
    SOAPRequest soapReq;
    soapReq.endpointUrl = req.controlUrl;
    soapReq.soapAction = "\"urn:schemas-upnp-org:service:RenderingControl:1#SetVolume\"";
    soapReq.contentType = "text/xml; charset=\"utf-8\"";
    soapReq.timeoutMs = 5000;
    soapReq.extraHeaders = { {"User-Agent", "NetDiscovery/1.0"}, {"Connection", "close"} };
    
    soapReq.xmlBody = 
        "<?xml version=\"1.0\"?>\r\n"
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
        "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
        "  <s:Body>\r\n"
        "    <u:SetVolume xmlns:u=\"urn:schemas-upnp-org:service:RenderingControl:1\">\r\n"
        "      <InstanceID>" + std::to_string(req.instanceID) + "</InstanceID>\r\n"
        "      <Channel>" + req.channel + "</Channel>\r\n"
        "      <DesiredVolume>" + std::to_string(req.targetVolume) + "</DesiredVolume>\r\n"
        "    </u:SetVolume>\r\n"
        "  </s:Body>\r\n"
        "</s:Envelope>\r\n";
    return soapReq;
}

SOAPRequest RenderingControlBuilder::BuildGetMute(const GetMuteRequest& req) {
    SOAPRequest soapReq;
    soapReq.endpointUrl = req.controlUrl;
    soapReq.soapAction = "\"urn:schemas-upnp-org:service:RenderingControl:1#GetMute\"";
    soapReq.contentType = "text/xml; charset=\"utf-8\"";
    soapReq.timeoutMs = 5000;
    soapReq.extraHeaders = { {"User-Agent", "NetDiscovery/1.0"}, {"Connection", "close"} };
    
    soapReq.xmlBody = 
        "<?xml version=\"1.0\"?>\r\n"
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
        "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
        "  <s:Body>\r\n"
        "    <u:GetMute xmlns:u=\"urn:schemas-upnp-org:service:RenderingControl:1\">\r\n"
        "      <InstanceID>" + std::to_string(req.instanceID) + "</InstanceID>\r\n"
        "      <Channel>" + req.channel + "</Channel>\r\n"
        "    </u:GetMute>\r\n"
        "  </s:Body>\r\n"
        "</s:Envelope>\r\n";
    return soapReq;
}

SOAPRequest RenderingControlBuilder::BuildSetMute(const SetMuteRequest& req) {
    SOAPRequest soapReq;
    soapReq.endpointUrl = req.controlUrl;
    soapReq.soapAction = "\"urn:schemas-upnp-org:service:RenderingControl:1#SetMute\"";
    soapReq.contentType = "text/xml; charset=\"utf-8\"";
    soapReq.timeoutMs = 5000;
    soapReq.extraHeaders = { {"User-Agent", "NetDiscovery/1.0"}, {"Connection", "close"} };
    
    std::string muteVal = req.muteStatus ? "1" : "0";
    
    soapReq.xmlBody = 
        "<?xml version=\"1.0\"?>\r\n"
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
        "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n"
        "  <s:Body>\r\n"
        "    <u:SetMute xmlns:u=\"urn:schemas-upnp-org:service:RenderingControl:1\">\r\n"
        "      <InstanceID>" + std::to_string(req.instanceID) + "</InstanceID>\r\n"
        "      <Channel>" + req.channel + "</Channel>\r\n"
        "      <DesiredMute>" + muteVal + "</DesiredMute>\r\n"
        "    </u:SetMute>\r\n"
        "  </s:Body>\r\n"
        "</s:Envelope>\r\n";
    return soapReq;
}

} // namespace NetDiscovery
