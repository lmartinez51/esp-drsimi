#pragma once

#include <string>

namespace semantic {

class LogicalEntityResolver {
public:
    /**
     * @brief Maps a logical entity name (e.g., "Netflix") to a canonical vendor-neutral identifier (e.g., "App_Netflix").
     * @param entityName The raw string entity name.
     * @return The canonical identifier.
     */
    std::string Resolve(const std::string& entityName) const;
};

} // namespace semantic
