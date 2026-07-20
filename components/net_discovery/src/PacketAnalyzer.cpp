/**
 * @file PacketAnalyzer.cpp
 * @brief Implementation of stateless SSDP / UPnP packet analysis utilities.
 *
 * No socket code. No filesystem code. No Windows SDK dependencies.
 * Only STL — fully portable to ESP-IDF.
 */

#include "../include/PacketAnalyzer.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace NetDiscovery {

// ============================================================
// PacketAnalyzer::DetectType()
// ============================================================

PacketType PacketAnalyzer::DetectType(const std::string& raw)
{
    if (raw.empty()) {
        return PacketType::Unknown;
    }

    // Extract the first line (up to \n).
    const auto eol = raw.find('\n');
    std::string firstLine = (eol != std::string::npos) ? raw.substr(0, eol) : raw;

    // Strip trailing \r.
    if (!firstLine.empty() && firstLine.back() == '\r') {
        firstLine.pop_back();
    }

    // Uppercase the first line for case-insensitive comparison.
    std::string upper = firstLine;
    std::transform(upper.begin(), upper.end(), upper.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

    if (upper.rfind("HTTP/",     0) == 0) return PacketType::HttpResponse;
    if (upper.rfind("NOTIFY ",   0) == 0) return PacketType::Notify;
    if (upper.rfind("M-SEARCH ", 0) == 0) return PacketType::MSearch;

    return PacketType::Unknown;
}

// ============================================================
// PacketAnalyzer::TypeToString()
// ============================================================

std::string PacketAnalyzer::TypeToString(PacketType type)
{
    switch (type) {
        case PacketType::HttpResponse: return "HTTP Response";
        case PacketType::Notify:       return "NOTIFY";
        case PacketType::MSearch:      return "M-SEARCH";
        case PacketType::Unknown:      return "Unknown";
    }
    return "Unknown";
}

// ============================================================
// PacketAnalyzer::ExtractHeaderValue()
// ============================================================

std::string PacketAnalyzer::ExtractHeaderValue(const std::string& raw,
                                                const std::string& headerName)
{
    // Build an uppercase version of the target header name.
    std::string targetUpper = headerName;
    std::transform(targetUpper.begin(), targetUpper.end(), targetUpper.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

    std::istringstream stream(raw);
    std::string line;
    bool firstLine = true;

    while (std::getline(stream, line)) {
        // Skip the request/status line.
        if (firstLine) {
            firstLine = false;
            continue;
        }

        // Strip trailing \r.
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        // Blank line signals end of headers.
        if (line.empty()) {
            break;
        }

        // Find the colon separating key from value.
        const auto colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            continue;
        }

        // Compare the key case-insensitively.
        std::string key = line.substr(0, colonPos);
        std::transform(key.begin(), key.end(), key.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

        if (key == targetUpper) {
            std::string value = line.substr(colonPos + 1);
            // Trim leading whitespace from the value.
            const auto start = value.find_first_not_of(" \t");
            return (start != std::string::npos) ? value.substr(start) : value;
        }
    }

    return {};
}

// ============================================================
// PacketAnalyzer::ExtractUuidFromUsn()
// ============================================================

std::string PacketAnalyzer::ExtractUuidFromUsn(const std::string& usn)
{
    // Find "uuid:" anywhere in the USN string.
    const auto uuidPos = usn.find("uuid:");
    if (uuidPos == std::string::npos) {
        return "unknown";
    }

    // Extract from "uuid:" to the end.
    std::string fromUuid = usn.substr(uuidPos);

    // Trim everything after "::" (which separates UUID from service URN).
    const auto sepPos = fromUuid.find("::");
    if (sepPos != std::string::npos) {
        fromUuid = fromUuid.substr(0, sepPos);
    }

    return fromUuid;  // e.g. "uuid:a1b2c3d4-e5f6-7890-abcd-ef1234567890"
}

// ============================================================
// PacketAnalyzer::ExtractUuid()
// ============================================================

std::string PacketAnalyzer::ExtractUuid(const std::string& raw)
{
    const std::string usn = ExtractHeaderValue(raw, "USN");
    if (usn.empty()) {
        return "unknown";
    }
    return ExtractUuidFromUsn(usn);
}

// ============================================================
// PacketAnalyzer::FormatTimestamp()
// ============================================================

std::string PacketAnalyzer::FormatTimestamp()
{
    const auto now = std::chrono::system_clock::now();
    const auto tt  = std::chrono::system_clock::to_time_t(now);
    const auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
                         now.time_since_epoch()) % 1000;

    std::tm tmBuf{};
#ifdef _WIN32
    localtime_s(&tmBuf, &tt);  // Thread-safe MSVC version.
#else
    localtime_r(&tt, &tmBuf);  // POSIX (future ESP-IDF / Linux).
#endif

    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%Y-%m-%d %H:%M:%S")
        << '.' << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}

// ============================================================
// PacketAnalyzer::FormatElapsed()
// ============================================================

std::string PacketAnalyzer::FormatElapsed(long long ms)
{
    if (ms < 1000LL) {
        return std::to_string(ms) + "ms";
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1)
        << (static_cast<double>(ms) / 1000.0) << "s";
    return oss.str();
}

} // namespace NetDiscovery
