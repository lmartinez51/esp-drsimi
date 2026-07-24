/**
 * @file ExecutionContext.h
 * @brief Environmental context container for execution planning (v5.0.0 Architecture Phase 8.5).
 * 
 * ExecutionContext provides runtime environmental details (user, permissions, battery, network, 
 * location) to the ExecutionPlanner without embedding business or protocol logic.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace NetDiscovery {
namespace Execution {

/**
 * @brief Environmental execution context model.
 */
struct ExecutionContext {
    std::string currentUser{"admin"};                                 // Invoking user ID or role
    std::vector<std::string> permissions{"control", "admin"};        // Active security permissions
    std::string networkState{"Connected"};                           // Network status ("Connected", "Degraded", "Offline")
    std::string batteryState{"Mains"};                               // Power mode ("Mains", "BatteryHigh", "BatteryLow")
    std::string location{"LivingRoom"};                             // Spatial location context
    std::string securityMode{"Normal"};                              // Security state ("Normal", "ArmAway", "Emergency")
    std::string executionMode{"Standard"};                            // Execution environment ("Standard", "DryRun", "Simulation")
    std::unordered_map<std::string, std::string> metadata;          // Extensible context metadata

    ExecutionContext() = default;
};

} // namespace Execution
} // namespace NetDiscovery
