/**
 * @file CapabilityTypes.h
 * @brief Capability & Action Semantic Domain Model Types (v5.0.0 Architecture Phase 7.5).
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <cstdint>

namespace NetDiscovery {

/**
 * @brief Strongly typed parameter types supported by operations.
 */
enum class ParameterType {
    Integer,
    Float,
    Boolean,
    String,
    Enum,
    Duration,
    Percentage,
    Color,
    JsonObject,
    Array,
    Void
};

inline std::string ToString(ParameterType type) {
    switch (type) {
        case ParameterType::Integer:    return "Integer";
        case ParameterType::Float:      return "Float";
        case ParameterType::Boolean:    return "Boolean";
        case ParameterType::String:     return "String";
        case ParameterType::Enum:       return "Enum";
        case ParameterType::Duration:   return "Duration";
        case ParameterType::Percentage: return "Percentage";
        case ParameterType::Color:      return "Color";
        case ParameterType::JsonObject: return "JsonObject";
        case ParameterType::Array:      return "Array";
        case ParameterType::Void:       return "Void";
        default:                        return "Unknown";
    }
}

/**
 * @brief Categorization for capabilities.
 */
enum class CapabilityCategory {
    Power,
    Media,
    Lighting,
    Climate,
    Security,
    Network,
    Sensor,
    AI,
    Display,
    Input,
    Storage,
    System,
    Custom
};

inline std::string ToString(CapabilityCategory cat) {
    switch (cat) {
        case CapabilityCategory::Power:    return "Power";
        case CapabilityCategory::Media:    return "Media";
        case CapabilityCategory::Lighting: return "Lighting";
        case CapabilityCategory::Climate:  return "Climate";
        case CapabilityCategory::Security: return "Security";
        case CapabilityCategory::Network:  return "Network";
        case CapabilityCategory::Sensor:   return "Sensor";
        case CapabilityCategory::AI:       return "AI";
        case CapabilityCategory::Display:  return "Display";
        case CapabilityCategory::Input:    return "Input";
        case CapabilityCategory::Storage:  return "Storage";
        case CapabilityCategory::System:   return "System";
        case CapabilityCategory::Custom:
        default:                           return "Custom";
    }
}

/**
 * @brief Reusable numerical, lexical, and logical constraint specifications for parameters.
 */
struct ParameterConstraint {
    std::optional<double> minimum;
    std::optional<double> maximum;
    std::optional<double> step;
    std::optional<size_t> minimumLength;
    std::optional<size_t> maximumLength;
    std::optional<std::string> regex;
    std::vector<std::string> allowedValues;
    std::string unit;
    std::optional<uint32_t> precision;
    std::unordered_map<std::string, std::string> metadata;
};

// Legacy compatibility alias
using ConstraintDefinition = ParameterConstraint;

/**
 * @brief Complete parameter specification for a capability operation.
 */
struct ParameterDefinition {
    std::string name;                                        // Unique parameter key within operation
    std::string displayName;                                 // Human-readable localized label
    std::string description;                                 // Parameter description
    ParameterType type{ParameterType::String};               // Strongly typed parameter type
    bool required{true};                                     // Mandatory flag
    std::string defaultValue;                                // Default fallback value serialized as string
    std::string unit;                                        // Measurement unit (e.g., "Kelvin", "dB", "%", "Hz")
    ParameterConstraint constraint;                          // Reusable constraint specification
    std::unordered_map<std::string, std::string> metadata;  // Extensible metadata
};

/**
 * @brief Canonical Operation Identifiers for standardized domain operations.
 */
namespace OperationId {
    constexpr const char* PowerOn          = "power.on";
    constexpr const char* PowerOff         = "power.off";
    constexpr const char* SetVolume        = "volume.set";
    constexpr const char* VolumeUp         = "volume.up";
    constexpr const char* VolumeDown       = "volume.down";
    constexpr const char* Mute             = "volume.mute";
    constexpr const char* Unmute           = "volume.unmute";
    constexpr const char* MediaPlay        = "media.play";
    constexpr const char* MediaPause       = "media.pause";
    constexpr const char* MediaStop        = "media.stop";
    constexpr const char* MediaNext        = "media.next";
    constexpr const char* MediaPrevious    = "media.previous";
    constexpr const char* SetBrightness    = "lighting.setBrightness";
    constexpr const char* SetColor         = "lighting.setColor";
    constexpr const char* SetTemperature   = "climate.setTemperature";
}

/**
 * @brief Expanded semantic operation/action model within a capability.
 */
struct OperationDefinition {
    std::string id;                                          // Canonical operation identifier (e.g., OperationId::MediaPlay)
    std::string name;                                        // Operation name (legacy alias for id/displayName)
    std::string displayName;                                 // Human-readable localized name
    std::string description;                                 // Detailed operation description
    std::vector<ParameterDefinition> parameters;             // Ordered parameter list
    std::unordered_map<std::string, ParameterDefinition> parameterMap; // O(1) param lookup by name
    ParameterType returnType{ParameterType::Boolean};        // Expected return parameter type
    bool readOnly{false};                                    // True if operation causes no state mutation
    uint32_t estimatedDurationMs{0};                         // Expected execution latency
    uint32_t timeoutMs{5000};                                // Max timeout before cancellation
    bool idempotent{true};                                   // True if multiple executions produce identical state
    bool safe{true};                                         // False for destructive actions (e.g., FactoryReset)
    bool requiresConfirmation{false};                        // True if UI/user confirmation is required before execution
    std::unordered_map<std::string, std::string> metadata;  // Extensible metadata

    void AddParameter(const ParameterDefinition& param) {
        parameters.push_back(param);
        if (!param.name.empty()) {
            parameterMap[param.name] = param;
        }
    }

    bool HasParameter(const std::string& paramName) const {
        return parameterMap.find(paramName) != parameterMap.end();
    }

    std::optional<ParameterDefinition> FindParameter(const std::string& paramName) const {
        auto it = parameterMap.find(paramName);
        if (it != parameterMap.end()) {
            return it->second;
        }
        return std::nullopt;
    }
};

} // namespace NetDiscovery
