/**
 * @file IPlanOptimizer.h
 * @brief Abstract interface for the optional Plan Optimizer stage.
 *
 * IPlanOptimizer sits between PlanBuilder and ExecutionPlanExecutor.
 * Its initial concrete implementation (PassThroughPlanOptimizer) returns the
 * plan unchanged. The interface exposes extension points for future optimizations:
 * constant folding, dead-branch elimination, duplicate action elimination, and
 * redundant delay removal.
 *
 * INVARIANT: Implementations MUST preserve observable execution behavior.
 * No optimization may change the externally visible execution outcome of a plan.
 *
 * ESP-Claw Platform — Phase E (Intent Compiler & End-to-End Integration)
 */

#pragma once

#include "plan/IExecutionPlan.h"
#include <memory>

namespace NetDiscovery {
namespace compiler {

/**
 * @brief Applies optional graph-level optimizations to a compiled IExecutionPlan.
 *
 * Implementation contract:
 *   - May return the original plan pointer unchanged (pass-through is always valid).
 *   - Must never modify the runtime; all modifications produce a new IExecutionPlan.
 *   - Must never change observable execution behavior (correct inputs produce identical outputs).
 *   - Concrete implementations should be stateless and thread-safe.
 */
class IPlanOptimizer {
public:
    virtual ~IPlanOptimizer() = default;

    /**
     * @brief Apply optimization passes to a compiled IExecutionPlan.
     * @param plan  The plan produced by IPlanBuilder.
     * @return The optimized plan (may be the same pointer if no optimization was applied).
     */
    virtual std::shared_ptr<Plan::IExecutionPlan> Optimize(
        std::shared_ptr<Plan::IExecutionPlan> plan
    ) const = 0;
};

} // namespace compiler
} // namespace NetDiscovery
