/**
 * @file ResolvedAdapter.h
 * @brief Immutable value object representing a successfully resolved protocol adapter (v5.0.0 Architecture Phase 10.1).
 */

#pragma once

#include "protocol/IProtocolAdapter.h"
#include "protocol/ProtocolAdapterDescriptor.h"
#include "protocol/ProtocolAdapterState.h"
#include "runtime/DispatcherCapabilities.h"

#include <memory>
#include <string>
#include <cstdint>

namespace NetDiscovery {
namespace Protocol {

/**
 * @brief Immutable value object returned inside AdapterResolutionResult.
 *
 * Holds everything required for execution and validation.
 * Contains ZERO execution methods.
 */
struct ResolvedAdapter {
    AdapterId                               adapterId;
    std::shared_ptr<IProtocolAdapter>       adapter;
    ProtocolAdapterDescriptor               descriptor;
    Runtime::DispatcherCapabilities         capabilities;
    ProtocolAdapterState                    state;
    uint64_t                                resolutionTimestamp{0};
    uint64_t                                resolutionVersion{0};

    ResolvedAdapter() = default;

    ResolvedAdapter(AdapterId id,
                    std::shared_ptr<IProtocolAdapter> ad,
                    ProtocolAdapterDescriptor desc,
                    Runtime::DispatcherCapabilities caps,
                    ProtocolAdapterState st,
                    uint64_t timestamp = 0,
                    uint64_t version = 1)
        : adapterId(std::move(id))
        , adapter(std::move(ad))
        , descriptor(std::move(desc))
        , capabilities(std::move(caps))
        , state(std::move(st))
        , resolutionTimestamp(timestamp)
        , resolutionVersion(version) {}

    bool IsValid() const { return adapter != nullptr && !adapterId.empty(); }
};

} // namespace Protocol
} // namespace NetDiscovery
