#pragma once

#include "../../../core/SOAPRequest.h"
#include "../../../../include/upnp/models/RenderingControlRequests.h"

namespace NetDiscovery {

/**
 * @brief Stateless builder for RenderingControl SOAP requests.
 * Generates valid SOAP envelopes based on explicitly provided parameters.
 */
class RenderingControlBuilder {
public:
    static SOAPRequest BuildGetVolume(const GetVolumeRequest& req);
    static SOAPRequest BuildSetVolume(const SetVolumeRequest& req);
    static SOAPRequest BuildGetMute(const GetMuteRequest& req);
    static SOAPRequest BuildSetMute(const SetMuteRequest& req);
};

} // namespace NetDiscovery
