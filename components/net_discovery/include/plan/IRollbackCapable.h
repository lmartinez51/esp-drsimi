/**
 * @file IRollbackCapable.h
 * @brief Opt-in compensation interface for rollbackable steps (v6.0 Phase C).
 */

#pragma once

#include "plan/ExecutionPlanContext.h"
#include "ExecutionInfrastructure.h"

namespace NetDiscovery {
namespace Plan {

class IRollbackCapable {
public:
    virtual ~IRollbackCapable() = default;

    virtual ExecutionResult Rollback(ExecutionPlanContext& context, ExecutionInfrastructure& infrastructure) = 0;
};

} // namespace Plan
} // namespace NetDiscovery
