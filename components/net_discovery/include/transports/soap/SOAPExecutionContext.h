#pragma once

#include "../../core/IExecutionContext.h"
#include "../../core/SOAPRequest.h"

namespace NetDiscovery {

/**
 * @brief Execution context carrying the fully constructed SOAP request payload.
 */
class SOAPExecutionContext : public IExecutionContext {
public:
    SOAPRequest request;

    explicit SOAPExecutionContext(const SOAPRequest& req) : request(req) {}
    explicit SOAPExecutionContext(SOAPRequest&& req) : request(std::move(req)) {}
};

} // namespace NetDiscovery