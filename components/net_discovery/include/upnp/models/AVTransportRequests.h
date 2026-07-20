#pragma once

#include <string>

namespace NetDiscovery {

struct PlayRequest {
    int instanceID;
    std::string controlUrl;
    std::string speed = "1";
};

struct PauseRequest {
    int instanceID;
    std::string controlUrl;
};

struct StopRequest {
    int instanceID;
    std::string controlUrl;
};

struct NextRequest {
    int instanceID;
    std::string controlUrl;
};

struct PreviousRequest {
    int instanceID;
    std::string controlUrl;
};

struct SeekRequest {
    int instanceID;
    std::string controlUrl;
    std::string unit = "REL_TIME"; // Common values: REL_TIME, TRACK_NR
    std::string target; // e.g., "00:15:00"
};

} // namespace NetDiscovery