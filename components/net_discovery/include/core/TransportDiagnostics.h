#pragma once

#include <string>

namespace NetDiscovery {

/**
 * @brief Represents diagnostics specifically owned by the Transport layer.
 * 
 * Transport-level diagnostics include HTTP status, timeouts, socket errors, and raw payloads.
 */
struct TransportDiagnostics {
    int httpStatusCode{0};
    std::string faultCode;       // e.g., "UPnPError 501"
    std::string rawPayload;      // Raw XML/JSON/etc. response for debugging
};

} // namespace NetDiscovery
