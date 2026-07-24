/**
 * @file ProtocolSessionContext.h
 * @brief Mutable runtime state for a protocol session (v5.0.0 Architecture Phase 12.1).
 */

#pragma once

#include "protocol/session/ProtocolSession.h"

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <utility>

namespace NetDiscovery {
namespace Protocol {

/**
 * @brief Mutable runtime state for a protocol session.
 *
 * Replaces protocol-specific mutable session state ownership inside adapters.
 */
struct ProtocolSessionContext {
    ProtocolSessionId sessionId;
    std::string       authenticationState{"Authenticated"};
    std::string       securityState{"Normal"};
    uint32_t          reconnectCounter{0};
    std::string       keepAliveState{"Active"};
    uint32_t          sequenceNumber{0};
    std::vector<uint32_t> pendingRequestIds;
    std::string       transactionState{"Idle"};
    uint64_t          expirationTimestampMs{0};
    uint64_t          lastActivityTimestampMs{0};
    uint64_t          stateVersion{0};
    std::unordered_map<std::string, std::string> sessionVariables;

    ProtocolSessionContext() = default;
    explicit ProtocolSessionContext(ProtocolSessionId id)
        : sessionId(std::move(id)) {}

    void NextSequence() { ++sequenceNumber; }
    void BumpVersion() { ++stateVersion; }
};

} // namespace Protocol
} // namespace NetDiscovery
