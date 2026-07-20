#pragma once

#include <string>

namespace NetDiscovery {

struct GetVolumeRequest {
    int instanceID;
    std::string controlUrl;
    std::string channel = "Master";
};

struct SetVolumeRequest {
    int instanceID;
    std::string controlUrl;
    int targetVolume;
    std::string channel = "Master";
};

struct GetMuteRequest {
    int instanceID;
    std::string controlUrl;
    std::string channel = "Master";
};

struct SetMuteRequest {
    int instanceID;
    std::string controlUrl;
    bool muteStatus;
    std::string channel = "Master";
};

} // namespace NetDiscovery