#include "semantic/LogicalEntityResolver.h"
#include <algorithm>
#include <cctype>

namespace semantic {

static std::string ToLower(const std::string& input) {
    std::string result = input;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    return result;
}

std::string LogicalEntityResolver::Resolve(const std::string& entityName) const {
    if (entityName.empty()) {
        return "";
    }

    std::string lowerEntity = ToLower(entityName);
    
    // Very basic mapping. In a real system, this would query a knowledge base.
    if (lowerEntity == "youtube") {
        return "App_YouTube";
    }
    if (lowerEntity == "netflix") {
        return "App_Netflix";
    }
    if (lowerEntity == "hulu") {
        return "App_Hulu";
    }
    if (lowerEntity == "spotify") {
        return "App_Spotify";
    }

    return "Entity_" + entityName;
}

} // namespace semantic
