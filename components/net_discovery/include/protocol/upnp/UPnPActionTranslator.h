/**
 * @file UPnPActionTranslator.h
 * @brief Pure semantic parameter translator (v5.0.0 Architecture Phase 11.1).
 */

#pragma once

#include "execution/ExecutionStep.h"

#include <string>
#include <unordered_map>

namespace NetDiscovery {
namespace Protocol {
namespace UPnP {

/**
 * @brief Translated semantic operation payload.
 */
struct UPnPActionTranslation {
    std::string operationName;
    std::unordered_map<std::string, std::string> arguments;
};

/**
 * @brief Pure translator converting ExecutionStep parameters into a UPnPActionTranslation.
 *
 * Performs ZERO XML serialization, ZERO HTTP formatting, and ZERO networking.
 */
class UPnPActionTranslator {
public:
    UPnPActionTranslator() = default;

    /**
     * @brief Translates an ExecutionStep into an operation name and argument map.
     */
    UPnPActionTranslation Translate(const Execution::ExecutionStep& step) const;
};

} // namespace UPnP
} // namespace Protocol
} // namespace NetDiscovery
