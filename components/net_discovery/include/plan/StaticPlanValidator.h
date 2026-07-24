/**
 * @file StaticPlanValidator.h
 * @brief Static graph structure validator (cycles, DAG integrity, orphans, reachability) (v6.0 Phase C.5).
 */

#pragma once

#include "plan/IExecutionGraph.h"
#include "plan/ValidationReport.h"

namespace NetDiscovery {
namespace Plan {

class StaticPlanValidator {
public:
    StaticPlanValidator() = default;

    /**
     * @brief Performs single-pass static verification of an IExecutionGraph.
     */
    ValidationReport ValidateGraph(const IExecutionGraph& graph) const;
};

} // namespace Plan
} // namespace NetDiscovery
