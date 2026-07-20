#include "../../include/transports/SOAPTransport.h"
#include "../../include/transports/soap/SOAPExecutionContext.h"
#include "../../include/parsing/soap/SOAPResponseDispatcher.h"
#include "../../include/HttpClient.h"
#include <iostream>
#include <memory>
#include <chrono>

extern bool g_verbose;

namespace NetDiscovery {

ExecutionResult SOAPTransport::Execute(const ExecutionRequest& request, const ExecutionRoute& route) {
    (void)request;
    ExecutionResult result;
    
    if (route.transport != TransportFamily::SOAP) {
        result.status = ExecutionStatus::TransportUnavailable;
        return result;
    }

    if (!route.executionContext) {
        if (g_verbose) std::cout << "[SOAPTransport] Error: No execution context provided in ExecutionRoute.\n";
        result.status = ExecutionStatus::ExecutionFailed;
        return result;
    }

    auto soapContext = std::static_pointer_cast<SOAPExecutionContext>(route.executionContext);
    if (!soapContext) {
        if (g_verbose) {
            std::cout << "[SOAPTransport] Error: Execution context is not a SOAPExecutionContext.\n";
        }
        result.status = ExecutionStatus::ExecutionFailed;
        return result;
    }

    const SOAPRequest& soapReq = soapContext->request;

    if (soapReq.endpointUrl.empty()) {
        if (g_verbose) std::cout << "[SOAPTransport] Bailing early: No valid endpoint URL provided in SOAPRequest.\n";
        result.status = ExecutionStatus::TransportUnavailable;
        result.errorMessage = "SOAP execution context lacked a valid endpoint URL.";
        return result;
    }

    if (g_verbose) {
        std::cout << "\n================ SOAP REQUEST ================\n";
        std::cout << "Endpoint:\n" << soapReq.endpointUrl << "\n\n";
        std::cout << "SOAPAction:\n" << soapReq.soapAction << "\n\n";
        std::cout << "Headers:\n";
        std::cout << "Content-Type: " << soapReq.contentType << "\n";
        std::cout << "SOAPAction: " << soapReq.soapAction << "\n";
        for (const auto& [k, v] : soapReq.extraHeaders) {
            std::cout << k << ": " << v << "\n";
        }
        std::cout << "\nBody:\n\n" << soapReq.xmlBody << "\n";
        std::cout << "==============================================\n\n";
        
        std::cout << "[SOAPTransport] Executing SOAP Request:\n";
        std::cout << "  Endpoint    : " << soapReq.endpointUrl << "\n";
        std::cout << "  SOAPAction  : " << soapReq.soapAction << "\n";
    }

    HttpClient client;
    
    // Merge required headers
    std::map<std::string, std::string> headers = soapReq.extraHeaders;
    headers["SOAPAction"] = soapReq.soapAction;
    if (!soapReq.contentType.empty()) {
        headers["Content-Type"] = soapReq.contentType;
    } else {
        headers["Content-Type"] = "text/xml; charset=\"utf-8\"";
    }

    auto startTime = std::chrono::steady_clock::now();
    auto httpResOpt = client.Post(soapReq.endpointUrl, soapReq.xmlBody, headers);
    
    auto endTime = std::chrono::steady_clock::now();
    result.elapsedTimeMs = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());

    if (!httpResOpt.has_value()) {
        if (g_verbose) std::cout << "[SOAPTransport] Network error/timeout\n";
        result.status = ExecutionStatus::TransportUnavailable;
        result.errorMessage = "SOAP Request failed (network error)";
        return result;
    }
    
    HttpResponse httpRes = httpResOpt.value();

    if (g_verbose) std::cout << "[SOAPTransport] HTTP POST returned status code: " << httpRes.statusCode << "\n";

    result.transportDiagnostics.httpStatusCode = httpRes.statusCode;
    result.transportDiagnostics.rawPayload = httpRes.body;

    // Map HTTP status to ExecutionStatus
    if (httpRes.statusCode == 200 || httpRes.statusCode == 201) {
        result.status = ExecutionStatus::Success;
        
        // Extract Service and Action from SOAPAction header
        // e.g. "urn:schemas-upnp-org:service:RenderingControl:1#GetVolume"
        std::string actionId = "Unknown";
        auto hashPos = soapReq.soapAction.find('#');
        if (hashPos != std::string::npos && hashPos + 1 < soapReq.soapAction.length()) {
            std::string rawAction = soapReq.soapAction.substr(hashPos + 1);
            // Strip trailing quotes if present
            if (!rawAction.empty() && rawAction.back() == '"') {
                rawAction.pop_back();
            }
            actionId = rawAction;
        }
        
        netdiscovery::soap::UPnPService serviceType = netdiscovery::soap::ParseServiceType(soapReq.soapAction);
        
        // Dispatch to Parser
        auto parsedResponse = netdiscovery::parsing::soap::SOAPResponseDispatcher::ParseResponse(serviceType, actionId, httpRes.body);
        
        result.parsedFields = parsedResponse.fields;
        result.parserDiagnostics = parsedResponse.diagnostics;
        
        if (!parsedResponse.success) {
            result.status = ExecutionStatus::ParseError;
            result.errorMessage = "Failed to parse SOAP response: " + parsedResponse.diagnostics.errorMessage;
        }
        
    } else if (httpRes.statusCode >= 400 && httpRes.statusCode < 500) {
        result.status = ExecutionStatus::ProtocolError;
        result.errorMessage = "HTTP " + std::to_string(httpRes.statusCode) + " Client Error";
    } else if (httpRes.statusCode >= 500) {
        // SOAP Faults typically return HTTP 500 Internal Server Error
        result.status = ExecutionStatus::ExecutionFailed;
        result.errorMessage = "HTTP 500 Server Error (SOAP Fault)";
        result.transportDiagnostics.faultCode = "UPnPError 501"; // Placeholder, real parsing of fault code can be added
    } else {
        result.status = ExecutionStatus::ExecutionFailed;
        result.errorMessage = "Unexpected HTTP status: " + std::to_string(httpRes.statusCode);
    }
    
    return result;
}

} // namespace NetDiscovery
