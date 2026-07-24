/**
 * @file IIntentCompiler.h
 * @brief Abstract interface for the Intent Compiler.
 *
 * IIntentCompiler transforms a structured IntentDocument (produced by the LLM layer)
 * into a semantic ASTNode tree. It must never parse free-form text or construct
 * runtime objects (ExecutionPlan, ActionStep, etc.).
 *
 * ESP-Claw Platform — Phase E (Intent Compiler & End-to-End Integration)
 */

#pragma once

#include "compiler/IntentDocument.h"
#include "compiler/IntentAST.h"
#include <memory>

namespace NetDiscovery {
namespace compiler {

/**
 * @brief Transforms a structured IntentDocument into a semantic ASTNode tree.
 *
 * Implementation contract:
 *   - Input: An IntentDocument produced by the LLM integration layer.
 *   - Output: A shared_ptr<ASTNode> root representing the resolved semantic tree.
 *   - The compiler must never perform natural-language parsing.
 *   - The compiler must never construct runtime plan objects (IExecutionPlan, ActionStep, etc.).
 *   - The compiler must never reference IDeviceController or BoundExecutionRequest.
 *   - If an actionName cannot be resolved to a known ActionId, return ASTNodeKind::Action
 *     with resolvedAction = ActionId::Unknown. Callers decide how to handle unknown actions.
 *   - Returns nullptr if the IntentDocument is structurally invalid or empty.
 */
class IIntentCompiler {
public:
    virtual ~IIntentCompiler() = default;

    /**
     * @brief Transform a structured IntentDocument into a semantic ASTNode tree.
     * @param doc The model-agnostic intent document produced by the LLM layer.
     * @return Root ASTNode of the compiled semantic tree, or nullptr on failure.
     */
    virtual std::shared_ptr<ASTNode> Compile(const IntentDocument& doc) const = 0;
};

} // namespace compiler
} // namespace NetDiscovery
