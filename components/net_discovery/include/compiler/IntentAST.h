/**
 * @file IntentAST.h
 * @brief Semantic intermediate representation produced by IIntentCompiler.
 *
 * IntentAST nodes represent the semantic workflow tree after action resolution.
 * They carry NO runtime objects: no BoundExecutionRequest, no IDeviceController,
 * no ExecutionPlan nodes. The PlanBuilder is the sole consumer of this tree and
 * is responsible for transforming it into immutable IExecutionPlan blueprints.
 *
 * ESP-Claw Platform — Phase E (Intent Compiler & End-to-End Integration)
 */

#pragma once

#include "core/ActionId.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>

namespace NetDiscovery {
namespace compiler {

/**
 * @brief Semantic node kinds in the resolved AST.
 */
enum class ASTNodeKind {
    Action,    ///< A single resolved device action
    Sequence,  ///< Ordered sequence of child AST nodes
    Branch,    ///< Boolean predicate branch (true/false children)
    Loop,      ///< Predicate-guarded repeat
    Parallel   ///< Concurrent execution of child AST nodes
};

/**
 * @brief A single node in the resolved semantic AST.
 *
 * Produced by IIntentCompiler::Compile(). Consumed by IPlanBuilder::Build().
 * Contains no runtime objects. Device resolution is recorded by reference string;
 * PlanBuilder matches it to a concrete LogicalDevice / IDeviceController.
 */
struct ASTNode {
    ASTNodeKind kind{ASTNodeKind::Action};

    /// Resolved ActionId from the IntentDocument actionName field.
    ActionId resolvedAction{ActionId::Unknown};

    /// Device reference string propagated from IntentDocument for PlanBuilder to resolve.
    std::string targetDeviceRef;

    /// Action parameters (string-typed; PlanBuilder may coerce).
    std::map<std::string, std::string> parameters;

    /// Symbolic predicate expression string (e.g. "volume > 40").
    /// PlanBuilder parses this into a concrete IPredicate.
    std::string conditionExpression;

    /// True-branch children (also used as Sequence / Parallel children list).
    std::vector<std::shared_ptr<ASTNode>> children;

    /// False / else-branch children for Branch nodes.
    std::vector<std::shared_ptr<ASTNode>> elseChildren;

    /// Maximum loop iterations for Loop nodes.
    int maxIterations{1};

    /// Whether failure of this node should be tolerated by the parent Sequence.
    bool isOptional{false};

    /// Wait duration to inject after this node as a DelayStep.
    /// 0 ms = no delay injected.
    std::chrono::milliseconds waitAfter{0};
};

} // namespace compiler
} // namespace NetDiscovery
