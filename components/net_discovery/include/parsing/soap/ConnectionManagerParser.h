#pragma once

#include "parsing/soap/ISOAPServiceParser.h"

namespace netdiscovery {
namespace parsing {
namespace soap {

class ConnectionManagerParser : public ISOAPServiceParser {
public:
    ParsedResponse Parse(const std::string& actionId, const tinyxml2::XMLDocument& xmlDoc) override;
};

} // namespace soap
} // namespace parsing
} // namespace netdiscovery
