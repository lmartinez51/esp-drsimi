/**
 * @file DefaultIntentCompiler.cpp
 * @brief Concrete IIntentCompiler implementation — transforms IntentDocument → ASTNode tree.
 *
 * ESP-Claw Platform — Phase E (Intent Compiler & End-to-End Integration)
 */

#include "compiler/DefaultIntentCompiler.h"
#include "semantic/IntentCanonicalizer.h"
#include "core/ActionId.h"
#include "esp_log.h"

static const char* TAG = "DefaultIntentCompiler";

namespace NetDiscovery {
namespace compiler {

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

std::shared_ptr<ASTNode> DefaultIntentCompiler::Compile(const IntentDocument& doc) const
{
    if (doc.intentId.empty() && doc.root.actionName.empty() &&
        doc.root.children.empty() && doc.root.kind == IntentNodeKind::SingleAction) {
        ESP_LOGE(TAG, "Compile: empty IntentDocument (intentId='%s')", doc.intentId.c_str());
        return nullptr;
    }

    ESP_LOGI(TAG, "Compile: intentId='%s' name='%s'",
             doc.intentId.c_str(), doc.intentName.c_str());

    return CompileNode(doc.root);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

std::shared_ptr<ASTNode> DefaultIntentCompiler::CompileNode(const IntentActionNode& node) const
{
    auto ast = std::make_shared<ASTNode>();

    // --- Map IntentNodeKind → ASTNodeKind -----------------------------------
    switch (node.kind) {
        case IntentNodeKind::SingleAction:
            ast->kind = ASTNodeKind::Action;
            break;
        case IntentNodeKind::Sequence:
            ast->kind = ASTNodeKind::Sequence;
            break;
        case IntentNodeKind::Condition:
            ast->kind = ASTNodeKind::Branch;
            break;
        case IntentNodeKind::Loop:
            ast->kind = ASTNodeKind::Loop;
            break;
        case IntentNodeKind::Parallel:
        case IntentNodeKind::MultiDeviceBroadcast:
            ast->kind = ASTNodeKind::Parallel;
            break;
    }

    // --- Resolve action name to ActionId ------------------------------------
    if (!node.actionName.empty()) {
        // 1. Instanciar el canonicalizador (ya que el método Normalize no es estático)
        semantic::IntentCanonicalizer canonicalizer;
        ActionId canonical = canonicalizer.Normalize(node.actionName);

        // 2. Si Normalize encuentra un alias, usarlo. Si regresa Unknown, usar FromString como fallback.
        ast->resolvedAction = (canonical != ActionId::Unknown) ? canonical : FromString(node.actionName);

        if (ast->resolvedAction == ActionId::Unknown && ast->kind == ASTNodeKind::Action) {
            ESP_LOGW(TAG, "CompileNode: unknown actionName='%s'", node.actionName.c_str());
        }
    }

    // --- Copy fields --------------------------------------------------------
    ast->targetDeviceRef      = node.targetDeviceRef;
    ast->parameters           = node.parameters;
    ast->conditionExpression  = node.conditionExpression;
    ast->maxIterations        = node.maxIterations;
    ast->isOptional           = node.isOptional;
    // waitAfter is not in IntentDocument; defaults to 0ms in ASTNode
    ast->waitAfter            = std::chrono::milliseconds(0);

    // --- Recursively compile children ---------------------------------------
    for (const auto& child : node.children) {
        auto childAst = CompileNode(child);
        if (childAst) {
            ast->children.push_back(std::move(childAst));
        }
    }

    for (const auto& child : node.elseChildren) {
        auto childAst = CompileNode(child);
        if (childAst) {
            ast->elseChildren.push_back(std::move(childAst));
        }
    }

    return ast;
}

} // namespace compiler
} // namespace NetDiscovery
