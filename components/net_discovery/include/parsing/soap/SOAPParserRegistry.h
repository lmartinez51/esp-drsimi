#pragma once

#include <memory>
#include <unordered_map>
#include "parsing/soap/ISOAPServiceParser.h"
#include "transports/soap/SOAPServiceTypes.h"

namespace netdiscovery {
namespace parsing {
namespace soap {

/**
 * @brief Registry that maps strongly typed UPnPService identifiers to their parsers.
 */
class SOAPParserRegistry {
public:
    static SOAPParserRegistry& GetInstance();

    void RegisterParser(netdiscovery::soap::UPnPService service, std::shared_ptr<ISOAPServiceParser> parser);
    std::shared_ptr<ISOAPServiceParser> GetParser(netdiscovery::soap::UPnPService service) const;

private:
    SOAPParserRegistry(); // Singleton pattern
    ~SOAPParserRegistry() = default;

    SOAPParserRegistry(const SOAPParserRegistry&) = delete;
    SOAPParserRegistry& operator=(const SOAPParserRegistry&) = delete;

    std::unordered_map<netdiscovery::soap::UPnPService, std::shared_ptr<ISOAPServiceParser>> m_parsers;
};

} // namespace soap
} // namespace parsing
} // namespace netdiscovery
