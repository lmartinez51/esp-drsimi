/**
 * @file HttpClient.h
 * @brief Minimal synchronous HTTP/1.1 client interface.
 */

#pragma once

#include <string>
#include <map>
#include <optional>
#include "esp_err.h"

namespace NetDiscovery {

struct HttpResponse {
    int statusCode = 0;
    std::map<std::string, std::string> headers;
    std::string body;
};

class HttpClient {
public:
    static constexpr int DEFAULT_TIMEOUT_SECONDS = 5;
    static constexpr int MAX_REDIRECTS = 3;

    HttpClient() = default;

    std::optional<HttpResponse> SendRequest(const std::string& method, const std::string& url, const std::string& body = "", const std::map<std::string, std::string>& extraHeaders = {});
    std::optional<HttpResponse> Get(const std::string& url, const std::map<std::string, std::string>& extraHeaders = {});
    std::optional<HttpResponse> Post(const std::string& url, const std::string& body = "", const std::map<std::string, std::string>& extraHeaders = {});
    std::optional<HttpResponse> Delete(const std::string& url, const std::map<std::string, std::string>& extraHeaders = {});

private:
    esp_err_t ParseUrl(const std::string& url,
                       std::string&       host,
                       int&               port,
                       std::string&       path) const;
};

} // namespace NetDiscovery
