/**
 * @file PassThroughPlanOptimizer.h
 * @brief Initial pass-through implementation of IPlanOptimizer.
 *
 * Returns the plan pointer unchanged. Provides override hooks for future
 * optimization passes (constant folding, dead-branch elimination, duplicate
 * action elimination, redundant delay removal).
 *
 * ESP-Claw Platform — Phase E (Intent Compiler & End-to-End Integration)
 */

#pragma once

#include "compiler/IPlanOptimizer.h"

namespace NetDiscovery {
namespace compiler {

class PassThroughPlanOptimizer : public IPlanOptimizer {
public:
    PassThroughPlanOptimizer() = default;

    /// Returns the plan unchanged.
    std::shared_ptr<Plan::IExecutionPlan> Optimize(
        std::shared_ptr<Plan::IExecutionPlan> plan
    ) const override;

protected:
    // Future optimization hooks — override in derived classes.
    virtual std::shared_ptr<Plan::IExecutionPlan> FoldConstants(
        std::shared_ptr<Plan::IExecutionPlan> plan) const { return plan; }
    virtual std::shared_ptr<Plan::IExecutionPlan> EliminateDeadBranches(
        std::shared_ptr<Plan::IExecutionPlan> plan) const { return plan; }
    virtual std::shared_ptr<Plan::IExecutionPlan> EliminateDuplicateActions(
        std::shared_ptr<Plan::IExecutionPlan> plan) const { return plan; }
    virtual std::shared_ptr<Plan::IExecutionPlan> RemoveRedundantDelays(
        std::shared_ptr<Plan::IExecutionPlan> plan) const { return plan; }
};

} // namespace compiler
} // namespace NetDiscovery
