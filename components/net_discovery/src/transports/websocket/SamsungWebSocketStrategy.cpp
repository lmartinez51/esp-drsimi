/**
 * @file SamsungWebSocketStrategy.cpp
 * @brief Constructs JSON payloads for Samsung WebSocket API.
 */

#include "../../../include/transports/websocket/SamsungWebSocketStrategy.h"
#include "../../../include/core/ExecutionRoute.h"
#include <sstream>
#include <vector>

namespace NetDiscovery {
namespace Strategy {

// Base64 encoding utility for the Samsung app name
static std::string Base64Encode(const std::vector<uint8_t>& data) {
    static const char encoding_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string encoded;
    int i = 0;
    uint32_t octet_a, octet_b, octet_c, triple;
    while (i < data.size()) {
        octet_a = i < data.size() ? data[i++] : 0;
        octet_b = i < data.size() ? data[i++] : 0;
        octet_c = i < data.size() ? data[i++] : 0;
        triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;
        encoded += encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded += encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded += i > data.size() + 1 ? '=' : encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded += i > data.size() ? '=' : encoding_table[(triple >> 0 * 6) & 0x3F];
    }
    return encoded;
}

std::string SamsungWebSocketStrategy::BuildKeyPayload(const std::string& keyName) const {
    std::stringstream ss;
    ss << "{\n"
       << "  \"method\": \"ms.remote.control\",\n"
       << "  \"params\": {\n"
       << "    \"Cmd\": \"Click\",\n"
       << "    \"DataOfCmd\": \"" << keyName << "\",\n"
       << "    \"Option\": \"false\",\n"
       << "    \"TypeOfRemote\": \"SendRemoteKey\"\n"
       << "  }\n"
       << "}";
    return ss.str();
}

std::string SamsungWebSocketStrategy::BuildWebSocketUrl(const std::string& token) const {
    std::string path = "/api/v2/channels/samsung.remote.control";
    if (!token.empty()) {
        path += "?token=" + token;
    } else {
        path += "?name=" + Base64Encode(std::vector<uint8_t>({'N','e','t','D','i','s','c','o','v','e','r','y'}));
    }
    return path;
}

void SamsungWebSocketStrategy::BuildRequest(ExecutionRequest& request, ExecutionRoute& route) const {
    std::string token = "";
    auto credIt = request.context.credentials.find("auth_token");
    if (credIt != request.context.credentials.end()) {
        token = credIt->second;
    }
    
    route.metadata["WebSocket-Path"] = BuildWebSocketUrl(token);

    auto GetKeyFromAction = [](const ActionDescriptor& action) -> std::string {
        if (action.id == ActionId::PowerOff) {
            return "KEY_POWER";
        } else if (action.id == ActionId::VolumeUp) {
            return "KEY_VOLUP";
        } else if (action.id == ActionId::VolumeDown) {
            return "KEY_VOLDOWN";
        } else if (action.id == ActionId::Mute) {
            return "KEY_MUTE";
        } else if (action.id == ActionId::SendKey) {
            return "KEY_UNKNOWN"; // Would be extracted from params in real implementation
        }
        return "";
    };

    std::string keyName = GetKeyFromAction(request.action);

    if (!keyName.empty()) {
        route.metadata["WebSocket-Payload"] = BuildKeyPayload(keyName);
    }
}

void SamsungWebSocketStrategy::ProcessResponse(ExecutionResult& result, const ExecutionContext& context) const {
    std::string response = result.transportDiagnostics.rawPayload;
    
    if (response.empty()) {
        return;
    }
    
    // Check if response contains a token
    if (response.find("\"token\":\"") != std::string::npos) {
        size_t pos = response.find("\"token\":\"") + 9;
        size_t end = response.find("\"", pos);
        if (end != std::string::npos) {
            std::string token = response.substr(pos, end - pos);
            result.parsedFields["auth_token"] = token;
            if (context.onCredentialUpdated) {
                context.onCredentialUpdated("auth_token", token);
            }
        }
    }
    
    if (response.find("ms.channel.unauthorized") != std::string::npos) {
        result.status = ExecutionStatus::AuthenticationRequired;
    }
}

} // namespace Strategy
} // namespace NetDiscovery
