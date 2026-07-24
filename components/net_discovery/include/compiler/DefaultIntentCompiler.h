/**
 * @file DefaultIntentCompiler.h
 * @brief Concrete implementation of IIntentCompiler.
 *
 * Transforms a structured IntentDocument into a semantic ASTNode tree by:
 *   1. Resolving actionName strings to ActionId values via FromString().
 *   2. Mapping IntentNodeKind values to ASTNodeKind values.
 *   3. Recursively compiling child IntentActionNodes.
 *
 * This compiler never parses free-form text and never constructs runtime objects.
 *
 * ESP-Claw Platform — Phase E (Intent Compiler & End-to-End Integration)
 */

#pragma once

#include "compiler/IIntentCompiler.h"

namespace NetDiscovery {
namespace compiler {

class DefaultIntentCompiler : public IIntentCompiler {
public:
    DefaultIntentCompiler() = default;

    std::shared_ptr<ASTNode> Compile(const IntentDocument& doc) const override;

private:
    /// Recursively compile an IntentActionNode into an ASTNode.
    std::shared_ptr<ASTNode> CompileNode(const IntentActionNode& node) const;
};

} // namespace compiler
} // namespace NetDiscovery
