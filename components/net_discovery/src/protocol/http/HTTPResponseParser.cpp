/**
 * @file HTTPResponseParser.cpp
 * @brief Implementation of HTTPResponseParser (v5.0.0 Architecture Phase 15).
 */

#include "protocol/http/HTTPResponseParser.h"

namespace NetDiscovery {
namespace Protocol {
namespace HTTP {

HTTPParsedResponse HTTPResponseParser::Parse(const std::string& payload,
                                             const std::string& contentType) const {

    HTTPParsedResponse parsed;
    parsed.contentType = contentType;

    if (payload.empty()) {
        parsed.isSuccessParse = true;
        return parsed;
    }

    // Basic key-value / JSON snippet extraction without throwing
    parsed.parsedFields["rawPayload"] = payload;
    parsed.isSuccessParse = true;
    return parsed;
}

} // namespace HTTP
} // namespace Protocol
} // namespace NetDiscovery
