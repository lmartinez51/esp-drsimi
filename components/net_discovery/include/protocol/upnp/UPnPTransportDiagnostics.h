/**
 * @file UPnPTransportDiagnostics.h
 * @brief Telemetry snapshot structure for transport-layer diagnostics (v5.0.0 Architecture Phase 11.1).
 */

#pragma once

#include <cstdint>

namespace NetDiscovery {
namespace Protocol {
namespace UPnP {

/**
 * @brief Dedicated value object owning transport-level diagnostic metrics.
 *
 * Populated by IUPnPTransport implementations. Consumed by UPnPAdapter.
 */
struct UPnPTransportDiagnostics {
    uint32_t dnsTimeMs{0};
    uint32_t connectTimeMs{0};
    uint32_t requestSizeBytes{0};
    uint32_t responseSizeBytes{0};
    uint32_t rttMs{0};
    uint32_t retriesCount{0};
    uint32_t reconnectCount{0};
    uint32_t timeoutCount{0};
};

} // namespace UPnP
} // namespace Protocol
} // namespace NetDiscovery
