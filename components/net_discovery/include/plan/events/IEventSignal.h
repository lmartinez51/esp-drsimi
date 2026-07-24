/**
 * @file IEventSignal.h
 * @brief Abstract event signal interface for EventWaitStep (v6.0 Phase D).
 */

#pragma once

#include "plan/events/WaitResult.h"
#include "plan/CancellationToken.h"
#include <chrono>

namespace NetDiscovery {
namespace Plan {

class IEventSignal {
public:
    virtual ~IEventSignal() = default;

    virtual WaitResult Wait(std::chrono::milliseconds timeout, CancellationToken cancelToken) = 0;
    virtual void Signal() = 0;
    virtual void Reset() = 0;
};

} // namespace Plan
} // namespace NetDiscovery
