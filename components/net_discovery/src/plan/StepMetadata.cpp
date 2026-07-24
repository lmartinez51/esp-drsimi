/**
 * @file StepMetadata.cpp
 * @brief Default fallback implementation of GetMetadata (v6.0 Phase D).
 */

#include "plan/steps/StepMetadata.h"
#include "plan/IExecutionStep.h"

namespace NetDiscovery {
namespace Plan {

static const StepMetadata s_defaultMetadata;

const StepMetadata& IExecutionStep::GetMetadata() const {
    return s_defaultMetadata;
}

} // namespace Plan
} // namespace NetDiscovery
