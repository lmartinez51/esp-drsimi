#pragma once

#include <string>
#include <map>
#include "semantic/SemanticDataModels.h"

namespace semantic {

class ParameterNormalizer {
public:
    /**
     * @brief Normalizes raw string parameters into typed variants.
     * @param rawParameters A map of raw string parameters (e.g., {"volume": "20", "mute": "true"})
     * @return A map of typed ExecutionParameterVariants.
     */
    std::map<std::string, ExecutionParameterVariant> Normalize(const std::map<std::string, std::string>& rawParameters) const;

private:
    ExecutionParameterVariant ParseValue(const std::string& value) const;
};

} // namespace semantic
