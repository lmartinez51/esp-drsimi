/**
 * @file CapabilityDefinition.h
 * @brief Protocol-independent Capability model (v5.0.0 Architecture Phase 7.5).
 * 
 * ====================================================================================
 * SEMANTIC DOMAIN ARCHITECTURE DOCUMENTATION
 * ====================================================================================
 * 
 * 1. CapabilityDefinition (represented by struct CapabilityModel):
 *    The static, protocol-agnostic archetype describing a cohesive set of device features
 *    (e.g., "Volume Control", "Media Playback", "Lighting"). It defines the available 
 *    operations, parameters, and metadata independently of how any single protocol 
 *    (UPnP, Matter, BLE, IR) implements them.
 * 
 * 2. CapabilityInstance:
 *    The dynamic, runtime state binding of a CapabilityDefinition to a specific physical 
 *    device endpoint. It tracks endpoint availability, transport protocols, active state,
 *    and execution constraints at runtime.
 * 
 * 3. OperationDefinition:
 *    A single invokable semantic action exposed by a capability (e.g., "volume.set", 
 *    "power.on"). Includes canonical OperationId, execution attributes (readOnly, idempotent,
 *    safe, duration, timeout), return types, and parameter definitions.
 * 
 * 4. ParameterDefinition:
 *    A strongly-typed argument specification required by an OperationDefinition. Exposes 
 *    type (Integer, Float, Boolean, String, etc.), required status, default fallback,
 *    measurement units, and a ParameterConstraint model.
 * 
 * 5. ParameterConstraint:
 *    Logical and numerical boundary specifications (min, max, step, regex, allowed values,
 *    precision) enforcing semantic validity on parameter inputs before execution.
 * 
 * ====================================================================================
 * FUTURE REGISTRY ARCHITECTURE (CapabilityDefinitionRegistry)
 * ====================================================================================
 * Currently, CapabilityModel instances are embedded directly as value objects inside 
 * KnowledgeEntity's CapabilitySet. To minimize memory overhead on resource-constrained 
 * microcontroller systems (e.g., ESP32 with limited SRAM), a future CapabilityDefinitionRegistry
 * will host shared canonical instances of CapabilityModel. KnowledgeEntities will then
 * reference registry flyweights or shared definitions while keeping value semantics.
 */

#pragma once

#include "core/CapabilityTypes.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace NetDiscovery {

/**
 * @brief Legacy capability enum kept purely as a backward-compatibility adapter.
 * @deprecated All internal platform components must consume CapabilityModel directly.
 */
enum class CapabilityType {
    Unknown,
    PowerControl,
    VolumeControl,
    Mute,
    MediaPlayback,
    MediaTransport,
    InputSelection,
    ScreenCasting,
    ApplicationLaunching,
    RemoteControl,
    Lighting,
    Brightness,
    ColorTemperature,
    ColorRGB,
    Sensor,
    Switch,
    Thermostat
};

/**
 * @brief Legacy adapter helper function converting CapabilityType enum to string representation.
 */
inline std::string ToString(CapabilityType cap) {
    switch (cap) {
        case CapabilityType::PowerControl:         return "Power Control";
        case CapabilityType::VolumeControl:        return "Volume Control";
        case CapabilityType::Mute:                 return "Mute";
        case CapabilityType::MediaPlayback:        return "Media Playback";
        case CapabilityType::MediaTransport:       return "Media Transport";
        case CapabilityType::InputSelection:       return "Input Selection";
        case CapabilityType::ScreenCasting:        return "Screen Casting";
        case CapabilityType::ApplicationLaunching: return "Application Launching";
        case CapabilityType::RemoteControl:        return "Remote Control";
        case CapabilityType::Lighting:             return "Lighting";
        case CapabilityType::Brightness:           return "Brightness";
        case CapabilityType::ColorTemperature:     return "Color Temperature";
        case CapabilityType::ColorRGB:             return "Color RGB";
        case CapabilityType::Sensor:               return "Sensor";
        case CapabilityType::Switch:               return "Switch";
        case CapabilityType::Thermostat:           return "Thermostat";
        case CapabilityType::Unknown:
        default:                                   return "Unknown";
    }
}

/**
 * @brief High-level semantic description of a device capability (CapabilityDefinition).
 */
struct CapabilityModel {
    std::string id;                                                   // Unique capability ID (e.g., "Volume Control")
    std::string displayName;                                          // Human-readable localized label
    std::string description;                                          // Detailed description
    CapabilityCategory category{CapabilityCategory::Custom};         // Functional categorization
    std::unordered_map<std::string, OperationDefinition> operations; // O(1) operation lookup by operation ID/name
    std::unordered_map<std::string, std::string> metadata;          // Extensible semantic metadata
    std::string version{"1.0.0"};                                     // Semantic model version

    CapabilityModel() = default;

    /**
     * @brief Backwards-compatibility constructor converting legacy CapabilityType to CapabilityModel.
     */
    CapabilityModel(CapabilityType legacyType) {
        id = NetDiscovery::ToString(legacyType);
        displayName = id;
        description = id;
    }

    CapabilityModel(int intVal) : CapabilityModel(static_cast<CapabilityType>(intVal)) {}
    CapabilityModel(long intVal) : CapabilityModel(static_cast<CapabilityType>(intVal)) {}

    /**
     * @brief Canonical constructor taking string ID.
     */
    CapabilityModel(const std::string& capId) : id(capId), displayName(capId), description(capId) {}

    bool operator==(const CapabilityModel& other) const {
        return id == other.id;
    }
    bool operator!=(const CapabilityModel& other) const {
        return id != other.id;
    }

    // Static legacy enum constants so Capability::VolumeControl works seamlessly
    static constexpr CapabilityType PowerControl         = CapabilityType::PowerControl;
    static constexpr CapabilityType VolumeControl        = CapabilityType::VolumeControl;
    static constexpr CapabilityType Mute                 = CapabilityType::Mute;
    static constexpr CapabilityType MediaPlayback        = CapabilityType::MediaPlayback;
    static constexpr CapabilityType MediaTransport       = CapabilityType::MediaTransport;
    static constexpr CapabilityType InputSelection       = CapabilityType::InputSelection;
    static constexpr CapabilityType ScreenCasting        = CapabilityType::ScreenCasting;
    static constexpr CapabilityType ApplicationLaunching = CapabilityType::ApplicationLaunching;
    static constexpr CapabilityType RemoteControl        = CapabilityType::RemoteControl;
    static constexpr CapabilityType Lighting             = CapabilityType::Lighting;
    static constexpr CapabilityType Brightness           = CapabilityType::Brightness;
    static constexpr CapabilityType ColorTemperature     = CapabilityType::ColorTemperature;
    static constexpr CapabilityType ColorRGB             = CapabilityType::ColorRGB;
    static constexpr CapabilityType Sensor               = CapabilityType::Sensor;
    static constexpr CapabilityType Switch               = CapabilityType::Switch;
    static constexpr CapabilityType Thermostat           = CapabilityType::Thermostat;
    static constexpr CapabilityType Unknown              = CapabilityType::Unknown;

    void AddOperation(const OperationDefinition& op) {
        std::string key = op.id.empty() ? op.name : op.id;
        if (!key.empty()) {
            operations[key] = op;
        }
    }

    bool HasOperation(const std::string& opIdOrName) const {
        return operations.find(opIdOrName) != operations.end();
    }

    std::optional<OperationDefinition> FindOperation(const std::string& opIdOrName) const {
        auto it = operations.find(opIdOrName);
        if (it != operations.end()) {
            return it->second;
        }
        return std::nullopt;
    }
};

/**
 * @brief Convert CapabilityModel to string (returns id).
 */
inline std::string ToString(const CapabilityModel& cap) {
    return cap.id.empty() ? "Unknown" : cap.id;
}

} // namespace NetDiscovery
