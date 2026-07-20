#pragma once

#include "../../../core/SOAPRequest.h"
#include "../../../../include/upnp/models/AVTransportRequests.h"

namespace NetDiscovery {

/**
 * @brief Stateless builder for AVTransport SOAP requests.
 * Generates valid SOAP envelopes based on explicitly provided parameters.
 */
class AVTransportBuilder {
public:
    static SOAPRequest BuildPlay(const PlayRequest& req);
    static SOAPRequest BuildPause(const PauseRequest& req);
    static SOAPRequest BuildStop(const StopRequest& req);
    static SOAPRequest BuildNext(const NextRequest& req);
    static SOAPRequest BuildPrevious(const PreviousRequest& req);
    static SOAPRequest BuildSeek(const SeekRequest& req);
};

} // namespace NetDiscovery