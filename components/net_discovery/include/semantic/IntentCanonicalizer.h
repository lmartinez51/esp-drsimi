#pragma once

#include <string>
#include "core/ActionId.h"

namespace semantic {

class IntentCanonicalizer {
public:
    /**
     * @brief Normalizes a raw string intent into a ActionId.
     * @param rawIntent The string intent (e.g., "turn_on", "power_on", "watch_netflix").
     * @return The corresponding ActionId, or ActionId::Unknown if not matched.
     */
    NetDiscovery::ActionId Normalize(const std::string& rawIntent) const;
};

} // namespace semantic
