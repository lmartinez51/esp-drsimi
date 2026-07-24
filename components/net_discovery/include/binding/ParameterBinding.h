/**
 * @file ParameterBinding.h
 * @brief Declarative parameter translation metadata model (v5.0.0 Architecture Phase 8.1).
 * 
 * Maps semantic parameter names (e.g., "level") to protocol payload keys (e.g., "DesiredVolume" 
 * in UPnP, "CurrentLevel" in Matter) without embedding execution logic.
 */

#pragma once

#include <string>
#include <unordered_map>

namespace NetDiscovery {
namespace Binding {

/**
 * @brief Formal parameter translation transformation rules.
 */
enum class ParameterTransform {
    Identity,      // Direct 1:1 mapping (no modification)
    Scale,         // Numerical range scaling (e.g. 0..100 to 0..255)
    Clamp,         // Bounded value clamping (min/max guard)
    BooleanInvert, // Logical negation (true -> false)
    LookupTable,   // Dictionary key-value map translation
    Expression,    // Declarative algebraic transform expression
    Custom         // Specialized adapter-level rule
};

/**
 * @brief Converts ParameterTransform enum to string representation.
 */
inline std::string ToString(ParameterTransform transform) {
    switch (transform) {
        case ParameterTransform::Identity:      return "Identity";
        case ParameterTransform::Scale:         return "Scale";
        case ParameterTransform::Clamp:         return "Clamp";
        case ParameterTransform::BooleanInvert: return "BooleanInvert";
        case ParameterTransform::LookupTable:   return "LookupTable";
        case ParameterTransform::Expression:    return "Expression";
        case ParameterTransform::Custom:        return "Custom";
        default:                                return "Unknown";
    }
}

/**
 * @brief Declarative metadata mapping between semantic parameter inputs and protocol parameters.
 */
struct ParameterBinding {
    std::string semanticParameter;                                    // Semantic parameter name (e.g. "level")
    std::string protocolParameter;                                    // Protocol payload key (e.g. "DesiredVolume")
    std::string defaultValue;                                         // Optional fallback default value
    ParameterTransform transformRule{ParameterTransform::Identity};   // Transformation rule enum
    std::unordered_map<std::string, std::string> transformMetadata;  // Transform rules (min, max, expression)
    bool required{true};                                              // Required parameter flag
    std::unordered_map<std::string, std::string> metadata;           // Extensible metadata

    bool operator==(const ParameterBinding& other) const {
        return semanticParameter == other.semanticParameter &&
               protocolParameter == other.protocolParameter;
    }
};

} // namespace Binding
} // namespace NetDiscovery
