/**
 * @file DefaultPlanBuilder.h
 * @brief Concrete implementation of IPlanBuilder.
 *
 * DefaultPlanBuilder owns all ExecutionPlan graph construction.
 * It walks the ASTNode tree depth-first and produces:
 *   - ActionStep for ASTNodeKind::Action (with BoundExecutionRequest, policy)
 *   - DelayStep for any ASTNode with waitAfter > 0ms
 *   - BranchStep for ASTNodeKind::Branch (with parsed IPredicate)
 *   - LoopStep for ASTNodeKind::Loop
 *   - ParallelStep for ASTNodeKind::Parallel
 *   - Sequential chaining for ASTNodeKind::Sequence
 * All resulting nodes are assembled into a DAGExecutionGraph and wrapped in
 * an immutable ExecutionPlan.
 *
 * ESP-Claw Platform — Phase E (Intent Compiler & End-to-End Integration)
 */

#pragma once

#include "compiler/IPlanBuilder.h"
#include "plan/DAGExecutionGraph.h"
#include "expressions/IPredicate.h"
#include <string>

namespace NetDiscovery {
namespace compiler {

class DefaultPlanBuilder : public IPlanBuilder {
public:
    DefaultPlanBuilder() = default;

    std::shared_ptr<Plan::IExecutionPlan> Build(
        const ASTNode&          root,
        const PlanBuildContext& ctx
    ) const override;

private:
    /// Recursively build graph nodes from an ASTNode subtree.
    /// Returns the ID of the last node in the subtree (used to chain edges).
    std::string BuildNode(
        const ASTNode&             node,
        const PlanBuildContext&    ctx,
        Plan::DAGExecutionGraph&   graph,
        int&                       idCounter,
        const std::string&         incomingEdgeSource
    ) const;

    /// Parse a symbolic predicate expression string into an IPredicate.
    /// Returns a ValuePredicate(true) for empty or unresolvable expressions.
    std::shared_ptr<Expressions::IPredicate> ParsePredicate(
        const std::string& conditionExpression
    ) const;

    /// Generate a unique node ID prefix (e.g. "action", "branch", "delay").
    std::string MakeId(const std::string& prefix, int counter) const;
};

} // namespace compiler
} // namespace NetDiscovery
