#pragma once

#include "../../../core/SOAPRequest.h"
#include <string>

namespace NetDiscovery {

struct GetProtocolInfoRequest {
    int instanceID = 0;
    std::string controlUrl;
};

class ConnectionManagerBuilder {
public:
    static SOAPRequest BuildGetProtocolInfo(const GetProtocolInfoRequest& req);
};

} // namespace NetDiscovery
