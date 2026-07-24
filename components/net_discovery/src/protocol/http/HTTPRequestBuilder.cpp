/**
 * @file HTTPRequestBuilder.cpp
 * @brief Implementation of HTTPRequestBuilder (v5.0.0 Architecture Phase 15).
 */

#include "protocol/http/HTTPRequestBuilder.h"

namespace NetDiscovery {
namespace Protocol {
namespace HTTP {

HTTPRequest HTTPRequestBuilder::BuildRequest(const Execution::ExecutionStep& step,
                                             const std::string& baseUrl) const {

    HTTPMethod method = HTTPMethod::GET;
    const std::string opId = step.GetOperationId();

    if (opId == "POST" || opId == "Publish") {
        method = HTTPMethod::POST;
    } else if (opId == "PUT" || opId == "Update") {
        method = HTTPMethod::PUT;
    } else if (opId == "DELETE" || opId == "Remove") {
        method = HTTPMethod::DELETE;
    } else if (opId == "PATCH") {
        method = HTTPMethod::PATCH;
    }

    std::string path;
    const auto& params = step.GetParameterValues();

    auto pathIt = params.find("path");
    if (pathIt != params.end()) {
        path = pathIt->second;
    } else {
        auto urlIt = params.find("url");
        if (urlIt != params.end()) path = urlIt->second;
    }

    std::string fullUrl = baseUrl;
    if (!path.empty()) {
        if (!fullUrl.empty() && fullUrl.back() == '/' && path.front() == '/') {
            fullUrl += path.substr(1);
        } else if (!fullUrl.empty() && fullUrl.back() != '/' && path.front() != '/') {
            fullUrl += "/" + path;
        } else {
            fullUrl += path;
        }
    }

    if (fullUrl.empty()) {
        fullUrl = "http://localhost/";
    }

    std::unordered_map<std::string, std::string> headers;
    auto hIt = params.find("contentType");
    if (hIt != params.end()) {
        headers["Content-Type"] = hIt->second;
    } else {
        headers["Content-Type"] = "application/json";
    }

    std::string body;
    auto bIt = params.find("body");
    if (bIt != params.end()) {
        body = bIt->second;
    } else {
        auto payloadIt = params.find("payload");
        if (payloadIt != params.end()) body = payloadIt->second;
    }

    uint32_t timeoutMs = step.GetTimeoutMs() > 0 ? step.GetTimeoutMs() : 5000;

    return HTTPRequest(method, fullUrl, headers, {}, body, timeoutMs);
}

} // namespace HTTP
} // namespace Protocol
} // namespace NetDiscovery
