#pragma once

#include <string>

namespace netdiscovery {
namespace parsing {

/**
 * @brief Represents diagnostics specifically owned by the Parser layer.
 * 
 * Transport-level diagnostics (HTTP status, timeout, TLS errors) are owned by the Transport layer.
 * ParserDiagnostics captures issues with interpreting the payload itself.
 */
struct ParserDiagnostics {
    bool hasError{false};
    std::string errorMessage;       // e.g., "Malformed XML document", "Missing mandatory node"
    
    // Optional: Could include node paths or line numbers in the future
};

} // namespace parsing
} // namespace netdiscovery
