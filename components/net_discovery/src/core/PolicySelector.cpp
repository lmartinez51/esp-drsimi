/**
 * @file PolicySelector.cpp
 * @brief Implementation of PolicySelector (v5.1.0 Phase B).
 */

#include "core/PolicySelector.h"

namespace NetDiscovery {

ExecutionPolicy PolicySelector::SelectPolicy(const PolicyContext& context) {
    if (context.interaction == InteractionType::Interactive) {
        return ExecutionPolicy::Interactive();
    } else if (context.interaction == InteractionType::Background) {
        return ExecutionPolicy::Background();
    }
    return ExecutionPolicy::FastFail();
}

} // namespace NetDiscovery
