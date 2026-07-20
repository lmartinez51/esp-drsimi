/**
 * @file ExecutionContext.h
 * @brief Placeholder for future Semantic Routing and Context-Aware Execution.
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>

namespace NetDiscovery {

/**
 * @brief Represents the situational awareness of a command being executed.
 * 
 * Note: This is a placeholder for Phase 9 (Semantic Execution).
 * It will eventually be populated by the LLM and passed down to
 * influence transport selection or command arguments (e.g. "turn on 
 * the lights" resolving to the lights in the currentRoom).
 */
struct ExecutionContext {
    std::string currentRoom;
    std::string conversationContext;
    std::vector<std::string> semanticScope;
    std::map<std::string, std::string> credentials;
    std::function<void(const std::string& key, const std::string& value)> onCredentialUpdated;
};

} // namespace NetDiscovery
