/**
 * @file DefaultPlanBuilder.cpp
 * @brief Concrete IPlanBuilder implementation.
 *
 * Transforms a semantic ASTNode tree into an immutable ExecutionPlan blueprint.
 * Owns all runtime object construction: steps, nodes, edges, and graph assembly.
 *
 * ESP-Claw Platform — Phase E (Intent Compiler & End-to-End Integration)
 */

#include "compiler/DefaultPlanBuilder.h"
#include "plan/ExecutionPlan.h"
#include "plan/ActionStep.h"
#include "plan/ControlSteps.h"
#include "plan/ExecutionNode.h"
#include "plan/ExecutionEdge.h"
#include "plan/steps/BranchStep.h"
#include "plan/steps/LoopStep.h"
#include "expressions/LiteralExpression.h"
#include "expressions/VariableExpression.h"
#include "expressions/BinaryExpression.h"
#include "expressions/LogicalPredicates.h"
#include "expressions/VariableRef.h"
#include "core/PolicyContext.h"
#include "core/PolicySelector.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <algorithm>
#include <cctype>
#include <cstring>

static const char* TAG = "DefaultPlanBuilder";

namespace NetDiscovery {
namespace compiler {

// ============================================================================
// Public API
// ============================================================================

std::shared_ptr<Plan::IExecutionPlan> DefaultPlanBuilder::Build(
    const ASTNode&          root,
    const PlanBuildContext& ctx) const
{
    auto graph = std::make_shared<Plan::DAGExecutionGraph>();
    int idCounter = 1;

    BuildNode(root, ctx, *graph, idCounter, "");

    auto plan = std::make_shared<Plan::ExecutionPlan>(
        ctx.planId.empty() ? ("plan-" + std::to_string(esp_timer_get_time())) : ctx.planId,
        ctx.planName.empty() ? "CompiledPlan" : ctx.planName,
        graph,
        ctx.policy
    );

    ESP_LOGI(TAG, "Build: planId='%s' nodes=%zu",
             plan->GetPlanId().c_str(),
             graph->GetNodes().size());

    return plan;
}

// ============================================================================
// Private helpers
// ============================================================================

std::string DefaultPlanBuilder::MakeId(const std::string& prefix, int counter) const
{
    return prefix + "-" + std::to_string(counter);
}

// ---------------------------------------------------------------------------
// ParsePredicate: convert symbolic condition expression → IPredicate
//
// Supported syntax patterns:
//   "var > literal"         → ComparePredicate(Variable, >, Literal)
//   "var < literal"         → ComparePredicate(Variable, <, Literal)
//   "var >= literal"        → ComparePredicate(Variable, >=, Literal)
//   "var <= literal"        → ComparePredicate(Variable, <=, Literal)
//   "var == 'literal'"      → ComparePredicate(Variable, ==, Literal)
//   "var != literal"        → ComparePredicate(Variable, !=, Literal)
//   "CONTAINS 'text'"       → ValuePredicate(true)  [future extension]
//   (empty / unrecognised)  → ValuePredicate(LiteralExpression(true))
// ---------------------------------------------------------------------------
std::shared_ptr<Expressions::IPredicate> DefaultPlanBuilder::ParsePredicate(
    const std::string& expr) const
{
    // Default: always-true predicate (safe fallback)
    auto alwaysTrue = std::make_shared<Expressions::ValuePredicate>(
        std::make_shared<Expressions::LiteralExpression>(Plan::ExecutionValue{true})
    );

    if (expr.empty()) {
        return alwaysTrue;
    }

    // Attempt to parse:  <varname> <op> <value>
    // Supported ops (ordered longest-first to avoid substring matches):
    struct OpEntry { const char* sym; Expressions::BinaryOperator op; };
    static const OpEntry kOps[] = {
        {">=", Expressions::BinaryOperator::GreaterOrEqual},
        {"<=", Expressions::BinaryOperator::LessOrEqual},
        {"!=", Expressions::BinaryOperator::NotEqual},
        {"==", Expressions::BinaryOperator::Equal},
        {">",  Expressions::BinaryOperator::Greater},
        {"<",  Expressions::BinaryOperator::Less},
    };

    for (const auto& entry : kOps) {
        auto pos = expr.find(entry.sym);
        if (pos == std::string::npos) continue;

        std::string lhs = expr.substr(0, pos);
        std::string rhs = expr.substr(pos + std::strlen(entry.sym));

        // Trim whitespace
        auto trim = [](std::string s) {
            while (!s.empty() && std::isspace((unsigned char)s.front())) s.erase(s.begin());
            while (!s.empty() && std::isspace((unsigned char)s.back()))  s.pop_back();
            return s;
        };
        lhs = trim(lhs);
        rhs = trim(rhs);

        if (lhs.empty() || rhs.empty()) continue;

        // LHS is always a variable reference (blackboard key)
        // VariableRef requires explicit construction — no default constructor.
        auto varExpr = std::make_shared<Expressions::VariableExpression>(
            Expressions::VariableRef(lhs));

        // RHS: attempt integer, then double, then string literal.
        // Use strtoll/strtod — no exceptions (ESP32 -fno-exceptions).
        std::shared_ptr<Expressions::IExpression> rhsExpr;

        // Strip surrounding quotes from string literals
        std::string rhsStripped = rhs;
        if (rhsStripped.size() >= 2 &&
            ((rhsStripped.front() == '\'' && rhsStripped.back() == '\'') ||
             (rhsStripped.front() == '"'  && rhsStripped.back() == '"'))) {
            rhsStripped = rhsStripped.substr(1, rhsStripped.size() - 2);
        }

        // Try integer via strtoll (no-exception safe)
        {
            char* endPtr = nullptr;
            long long iv = std::strtoll(rhs.c_str(), &endPtr, 10);
            if (endPtr != rhs.c_str() && endPtr == rhs.c_str() + rhs.size()) {
                rhsExpr = std::make_shared<Expressions::LiteralExpression>(
                    Plan::ExecutionValue{static_cast<int64_t>(iv)});
            }
        }

        // Try double via strtod if integer parse failed
        if (!rhsExpr) {
            char* endPtr = nullptr;
            double dv = std::strtod(rhs.c_str(), &endPtr);
            if (endPtr != rhs.c_str() && endPtr == rhs.c_str() + rhs.size()) {
                rhsExpr = std::make_shared<Expressions::LiteralExpression>(
                    Plan::ExecutionValue{dv});
            }
        }

        // Fall back to string literal
        if (!rhsExpr) {
            rhsExpr = std::make_shared<Expressions::LiteralExpression>(
                Plan::ExecutionValue{std::string(rhsStripped)});
        }

        return std::make_shared<Expressions::ComparePredicate>(varExpr, entry.op, rhsExpr);
    }

    // Unrecognised expression — log a warning and return always-true
    ESP_LOGW(TAG, "ParsePredicate: could not parse expression '%s', using always-true", expr.c_str());
    return alwaysTrue;
}


// ---------------------------------------------------------------------------
// BuildNode: recursively build graph nodes from an ASTNode subtree.
// Returns the ID of the last node emitted (for edge chaining by callers).
// ---------------------------------------------------------------------------
std::string DefaultPlanBuilder::BuildNode(
    const ASTNode&             node,
    const PlanBuildContext&    ctx,
    Plan::DAGExecutionGraph&   graph,
    int&                       idCounter,
    const std::string&         incomingEdgeSource) const
{
    // Helper: add an OnSuccess edge from src → tgt
    auto addEdge = [&](const std::string& src, const std::string& tgt,
                       Plan::ExecutionEdgeType type = Plan::ExecutionEdgeType::OnSuccess,
                       const std::string& condKey = "") {
        if (src.empty() || tgt.empty()) return;
        Plan::ExecutionEdge edge;
        edge.sourceNodeId  = src;
        edge.targetNodeId  = tgt;
        edge.type          = type;
        edge.conditionKey  = condKey;
        graph.AddEdge(edge);
    };

    // Helper: add an action step + node to graph, return its ID
    auto addAction = [&](const std::string& stepId,
                         const std::string& name,
                         const ASTNode&     n) -> std::string {
        BoundExecutionRequest req;
        req.targetDevice       = ctx.resolvedDevice;
        req.selectedController = ctx.selectedController;
        req.action.id          = n.resolvedAction;
        req.action.displayName = ToString(n.resolvedAction);
        req.parameters         = n.parameters;

        // Per-action policy from PolicySelector
        PolicyContext pctx   = PolicyContext::FromAction(req.action);
        req.policy           = PolicySelector::SelectPolicy(pctx);

        auto step = std::make_shared<Plan::ActionStep>(stepId, name, req);
        graph.AddNode(std::make_shared<Plan::ExecutionNode>(step));
        return stepId;
    };

    std::string lastId;

    switch (node.kind) {

    // -----------------------------------------------------------------------
    case ASTNodeKind::Action: {
        int id = idCounter++;
        std::string stepId = MakeId("action", id);
        addAction(stepId, ToString(node.resolvedAction), node);
        addEdge(incomingEdgeSource, stepId);
        lastId = stepId;

        // Inject post-action delay if requested
        if (node.waitAfter.count() > 0) {
            int did = idCounter++;
            std::string delayId = MakeId("delay", did);
            auto delay = std::make_shared<Plan::DelayStep>(
                delayId, "PostActionDelay", node.waitAfter);
            graph.AddNode(std::make_shared<Plan::ExecutionNode>(delay));
            addEdge(stepId, delayId);
            lastId = delayId;
        }
        break;
    }

    // -----------------------------------------------------------------------
    case ASTNodeKind::Sequence: {
        std::string prev = incomingEdgeSource;
        for (const auto& child : node.children) {
            if (child) {
                prev = BuildNode(*child, ctx, graph, idCounter, prev);
            }
        }
        lastId = prev;
        break;
    }

    // -----------------------------------------------------------------------
    case ASTNodeKind::Branch: {
        // 1. Emit BranchStep
        int bid = idCounter++;
        std::string branchId = MakeId("branch", bid);
        auto predicate = ParsePredicate(node.conditionExpression);
        auto branchStep = std::make_shared<Plan::BranchStep>(
            branchId, "Branch_" + node.conditionExpression, predicate);
        graph.AddNode(std::make_shared<Plan::ExecutionNode>(branchStep));
        addEdge(incomingEdgeSource, branchId);

        // 2. True subtree
        for (const auto& child : node.children) {
            if (child) {
                BuildNode(*child, ctx, graph, idCounter, "");
                addEdge(branchId,
                        graph.GetNodes().back()->GetNodeId(),
                        Plan::ExecutionEdgeType::OnCondition,
                        branchId + ".true");
            }
        }

        // 3. False / else subtree
        for (const auto& child : node.elseChildren) {
            if (child) {
                BuildNode(*child, ctx, graph, idCounter, "");
                addEdge(branchId,
                        graph.GetNodes().back()->GetNodeId(),
                        Plan::ExecutionEdgeType::OnCondition,
                        branchId + ".false");
            }
        }

        lastId = branchId;
        break;
    }

    // -----------------------------------------------------------------------
    case ASTNodeKind::Loop: {
        int lid = idCounter++;
        std::string loopId = MakeId("loop", lid);
        auto predicate = ParsePredicate(node.conditionExpression);
        uint8_t maxIter = static_cast<uint8_t>(
            std::max(1, std::min(node.maxIterations, 255)));
        auto loopStep = std::make_shared<Plan::LoopStep>(
            loopId, "Loop", predicate, maxIter);
        graph.AddNode(std::make_shared<Plan::ExecutionNode>(loopStep));
        addEdge(incomingEdgeSource, loopId);

        // Build loop body (children)
        for (const auto& child : node.children) {
            if (child) {
                BuildNode(*child, ctx, graph, idCounter, loopId);
            }
        }

        lastId = loopId;
        break;
    }

    // -----------------------------------------------------------------------
    case ASTNodeKind::Parallel: {
        // Build child steps as independent ActionStep objects passed to ParallelStep
        std::vector<std::shared_ptr<Plan::IExecutionStep>> childSteps;

        for (const auto& child : node.children) {
            if (!child || child->kind != ASTNodeKind::Action) {
                ESP_LOGW(TAG, "BuildNode(Parallel): only Action children supported; skipping");
                continue;
            }
            int cid = idCounter++;
            std::string cId = MakeId("par-action", cid);
            BoundExecutionRequest req;
            req.targetDevice       = ctx.resolvedDevice;
            req.selectedController = ctx.selectedController;
            req.action.id          = child->resolvedAction;
            req.action.displayName = ToString(child->resolvedAction);
            req.parameters         = child->parameters;
            PolicyContext pctx     = PolicyContext::FromAction(req.action);
            req.policy             = PolicySelector::SelectPolicy(pctx);
            childSteps.push_back(
                std::make_shared<Plan::ActionStep>(cId, ToString(child->resolvedAction), req));
        }

        int pid = idCounter++;
        std::string parId = MakeId("parallel", pid);
        auto parallelStep = std::make_shared<Plan::ParallelStep>(
            parId, "Parallel", std::move(childSteps), Plan::ParallelPolicy::WaitAll);
        graph.AddNode(std::make_shared<Plan::ExecutionNode>(parallelStep));
        addEdge(incomingEdgeSource, parId);
        lastId = parId;
        break;
    }

    default:
        ESP_LOGW(TAG, "BuildNode: unhandled ASTNodeKind %d", static_cast<int>(node.kind));
        lastId = incomingEdgeSource;
        break;
    }

    return lastId;
}

} // namespace compiler
} // namespace NetDiscovery
