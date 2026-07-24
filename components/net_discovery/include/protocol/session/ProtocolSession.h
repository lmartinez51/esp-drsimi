/**
 * @file ProtocolSession.h
 * @brief Immutable identity value object representing a protocol session (v5.0.0 Architecture Phase 12.1).
 */

#pragma once

#include <string>
#include <cstdint>
#include <utility>

namespace NetDiscovery {
namespace Protocol {

using ProtocolSessionId = std::string;

/**
 * @brief Immutable identity value object representing a protocol session.
 *
 * Contains ZERO mutable runtime state. Pure value object.
 */
struct ProtocolSession {
    ProtocolSessionId sessionId;
    std::string       adapterId;
    std::string       protocol;
    std::string       targetEndpoint;
    uint64_t          creationTimestampMs{0};
    uint64_t          generationVersion{1};

    ProtocolSession() = default;

    ProtocolSession(ProtocolSessionId sid,
                    std::string aid,
                    std::string proto,
                    std::string endp,
                    uint64_t created = 0,
                    uint64_t gen = 1)
        : sessionId(std::move(sid))
        , adapterId(std::move(aid))
        , protocol(std::move(proto))
        , targetEndpoint(std::move(endp))
        , creationTimestampMs(created)
        , generationVersion(gen) {}

    bool IsValid() const { return !sessionId.empty() && !protocol.empty() && !targetEndpoint.empty(); }
};

} // namespace Protocol
} // namespace NetDiscovery
