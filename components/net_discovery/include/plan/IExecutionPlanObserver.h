/**
 * @file IExecutionPlanObserver.h
 * @brief Observer interface for listening to execution events (v6.0 Phase C).
 */

#pragma once

#include "plan/ExecutionEvent.h"

namespace NetDiscovery {
namespace Plan {

class IExecutionPlanObserver {
public:
    virtual ~IExecutionPlanObserver() = default;

    virtual void OnExecutionEvent(const ExecutionEvent& event) = 0;
};

} // namespace Plan
} // namespace NetDiscovery
