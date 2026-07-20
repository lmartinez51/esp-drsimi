#pragma once

#include <string>
#include <unordered_map>
#include "parsing/ParserDiagnostics.h"

namespace netdiscovery {
namespace parsing {

/**
 * @brief An intermediate protocol-neutral model representing the output of a parser.
 * 
 * This model is explicitly an intermediate representation. 
 * Future phases will evolve to service-specific response models (e.g. RenderingControlResponse), 
 * where the Semantic Layer consumes `fields` to construct strongly typed models.
 */
struct ParsedResponse {
    bool success{false};
    
    // Extracted key-value pairs (e.g., "CurrentVolume" -> "15")
    std::unordered_map<std::string, std::string> fields;
    
    // Parser-specific diagnostics
    ParserDiagnostics diagnostics;
};

} // namespace parsing
} // namespace netdiscovery
