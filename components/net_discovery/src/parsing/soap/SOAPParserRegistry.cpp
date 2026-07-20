#include "parsing/soap/SOAPParserRegistry.h"
#include "parsing/soap/RenderingControlParser.h"
#include "parsing/soap/AVTransportParser.h"
#include "parsing/soap/ConnectionManagerParser.h"

namespace netdiscovery {
namespace parsing {
namespace soap {

SOAPParserRegistry& SOAPParserRegistry::GetInstance() {
    static SOAPParserRegistry instance;
    return instance;
}

SOAPParserRegistry::SOAPParserRegistry() {
    // Register parsers during framework initialization
    RegisterParser(netdiscovery::soap::UPnPService::RenderingControl, std::make_shared<RenderingControlParser>());
    RegisterParser(netdiscovery::soap::UPnPService::AVTransport, std::make_shared<AVTransportParser>());
    RegisterParser(netdiscovery::soap::UPnPService::ConnectionManager, std::make_shared<ConnectionManagerParser>());
}

void SOAPParserRegistry::RegisterParser(netdiscovery::soap::UPnPService service, std::shared_ptr<ISOAPServiceParser> parser) {
    if (parser) {
        m_parsers[service] = std::move(parser);
    }
}

std::shared_ptr<ISOAPServiceParser> SOAPParserRegistry::GetParser(netdiscovery::soap::UPnPService service) const {
    auto it = m_parsers.find(service);
    if (it != m_parsers.end()) {
        return it->second;
    }
    return nullptr;
}

} // namespace soap
} // namespace parsing
} // namespace netdiscovery
