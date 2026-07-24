/**
 * @file PolicySelector.h
 * @brief PolicySelector component mapping PolicyContext to ExecutionPolicy (v5.1.0 Phase B).
 */

#pragma once

#include "core/ExecutionPolicy.h"
#include "core/PolicyContext.h"

namespace NetDiscovery {

class PolicySelector {
public:
    static ExecutionPolicy SelectPolicy(const PolicyContext& context);
};

} // namespace NetDiscovery
