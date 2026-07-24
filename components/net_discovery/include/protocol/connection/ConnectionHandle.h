/**
 * @file ConnectionHandle.h
 * @brief Immutable value object representing a leased connection handle (v5.0.0 Architecture Phase 11.2).
 */

#pragma once

#include <string>
#include <cstdint>
#include <utility>

namespace NetDiscovery {
namespace Protocol {

using ConnectionId = std::string;

/**
 * @brief Immutable lightweight value object representing a leased connection.
 *
 * Contains ZERO socket handles or transport implementations. Pure value object.
 */
struct ConnectionHandle {
    ConnectionId connectionId;
    std::string  protocol;
    std::string  endpoint;
    uint64_t     creationTimestampMs{0};
    uint64_t     lastActivityTimestampMs{0};
    uint64_t     generationVersion{0};

    ConnectionHandle() = default;

    ConnectionHandle(ConnectionId id,
                     std::string proto,
                     std::string endp,
                     uint64_t created = 0,
                     uint64_t lastAct = 0,
                     uint64_t gen = 1)
        : connectionId(std::move(id))
        , protocol(std::move(proto))
        , endpoint(std::move(endp))
        , creationTimestampMs(created)
        , lastActivityTimestampMs(lastAct)
        , generationVersion(gen) {}

    bool IsValid() const { return !connectionId.empty() && !endpoint.empty(); }
};

} // namespace Protocol
} // namespace NetDiscovery
