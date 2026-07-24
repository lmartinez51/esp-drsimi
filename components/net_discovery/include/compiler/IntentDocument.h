/**
 * @file IntentDocument.h
 * @brief Model-agnostic boundary object between the LLM integration layer and the Intent Compiler.
 *
 * IntentDocument carries structured intent information produced by the LLM after natural-language
 * understanding. It deliberately contains no OpenAI, tool-calling, or prompt-specific concepts.
 * Every field below this boundary is provider-neutral.
 *
 * ESP-Claw Platform — Phase E (Intent Compiler & End-to-End Integration)
 */

#pragma once

#include <string>
#include <vector>
#include <map>

namespace NetDiscovery {
namespace compiler {

/**
 * @brief Structural kinds of intent nodes supported by the compiler.
 */
enum class IntentNodeKind {
    SingleAction,        ///< One discrete device command (e.g. PowerOn, SetVolume)
    Sequence,            ///< Ordered list of child nodes executed left-to-right
    Condition,           ///< If-else branch evaluated at runtime
    Loop,                ///< Repeat child node while a condition holds
    Parallel,            ///< All children execute concurrently
    MultiDeviceBroadcast ///< Same action sent to multiple resolved device targets
};

/**
 * @brief A single node in the flat IntentDocument tree.
 *
 * Nodes may nest recursively via @c children / @c elseChildren.
 * No runtime objects (BoundExecutionRequest, IDeviceController, etc.)
 * may appear here; this is strictly a declarative, model-agnostic document.
 */
struct IntentActionNode {
    IntentNodeKind kind{IntentNodeKind::SingleAction};

    /// Canonical action name, e.g. "PowerOn", "SetVolume", "LaunchApplication".
    /// Must match a known ActionId string. The compiler resolves this to ActionId.
    std::string actionName;

    /// Human-readable device description used by DeviceMatcher to resolve a LogicalDevice.
    /// Examples: "living room TV", "tv", "Samsung TV".
    std::string targetDeviceRef;

    /// Action parameters. Keys and values are plain strings; the PlanBuilder converts types.
    std::map<std::string, std::string> parameters;

    /// Symbolic predicate expression, used for Condition / Loop nodes.
    /// Example: "volume > 40", "power_state == 'OFF'", "installed_apps CONTAINS 'Netflix'".
    /// PlanBuilder parses this into an IPredicate at build time.
    std::string conditionExpression;

    /// Child nodes for Sequence, Condition (true branch), Loop body, or Parallel children.
    std::vector<IntentActionNode> children;

    /// False / else branch for Condition nodes.
    std::vector<IntentActionNode> elseChildren;

    /// Maximum loop iterations. Only meaningful for Loop nodes.
    int maxIterations{1};

    /// If true, failure of this node does not abort the parent Sequence.
    bool isOptional{false};
};

/**
 * @brief Top-level structured intent document produced by the LLM integration layer.
 *
 * This is the sole input type accepted by IIntentCompiler.
 * The LLM is responsible for populating this structure from natural-language input.
 */
struct IntentDocument {
    /// Caller-assigned correlation identifier (e.g. UUID or request ID).
    std::string intentId;

    /// Human-readable label for diagnostics and telemetry (e.g. "TurnOnAndYouTube").
    std::string intentName;

    /// Root intent node representing the entire workflow tree.
    IntentActionNode root;
};

} // namespace compiler
} // namespace NetDiscovery
