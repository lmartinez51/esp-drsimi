/**
 * @file Observation.h
 * @brief Protocol-independent observation model for ESP-Claw Entity Resolution Engine.
 */

#pragma once

#include "core/DiscoverySource.h"
#include "core/ProtocolEndpoint.h"
#include "core/Capability.h"
#include "core/ControllerCandidate.h"

#include <string>
#include <vector>

namespace NetDiscovery {

/**
 * @brief Represents a raw observation emitted by a protocol stack (SSDP, mDNS, BLE, Matter, IR).
 * Contains only objective facts without AI annotations, persistence IDs, or lifecycle state.
 */
struct Observation {
    DiscoverySource source{DiscoverySource::Unknown};

    // Hardware & Network Identity Candidates
    std::string macAddress;
    std::string serialNumber;
    std::string vendor;
    std::string model;
    std::string hostname;
    std::string ip;
    std::string usn;            // UPnP Unique Service Name / UUID
    std::string matterNodeId;   // Matter Operational Node ID
    std::string bleIdentity;    // BLE MAC or IRK identity
    std::string userAlias;      // User or system provided alias

    // Protocol & Dynamic Endpoint Evidence
    std::vector<ProtocolEndpoint> endpoints;
    std::vector<Capability> capabilities;
    std::vector<ControllerCandidate> controllers;
    std::vector<std::string> rawTags;
};

} // namespace NetDiscovery
