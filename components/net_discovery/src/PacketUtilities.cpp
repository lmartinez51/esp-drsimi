/**
 * @file PacketUtilities.cpp
 * @brief Implementation of generic packet analysis helper functions.
 *
 * Renamed from PacketAnalyzer.cpp. No protocol-specific logic.
 * STL only — fully portable to ESP-IDF.
 */

#include "../include/PacketUtilities.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace NetDiscovery {

// ============================================================
// PacketUtilities::DetectType()
// ============================================================

PacketType PacketUtilities::DetectType(const std::string& raw)
{
    if (raw.empty()) return PacketType::Unknown;

    // Extract the first line (up to \n).
    const auto eol = raw.find('\n');
    std::string firstLine = (eol != std::string::npos) ? raw.substr(0, eol) : raw;

    // Strip trailing \r.
    if (!firstLine.empty() && firstLine.back() == '\r') {
        firstLine.pop_back();
    }

    // Upper-case for comparison.
    std::string upper = firstLine;
    std::transform(upper.begin(), upper.end(), upper.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

    if (upper.rfind("HTTP/",     0) == 0) return PacketType::HttpResponse;
    if (upper.rfind("NOTIFY ",   0) == 0) return PacketType::Notify;
    if (upper.rfind("M-SEARCH ", 0) == 0) return PacketType::MSearch;

    return PacketType::Unknown;
}

// ============================================================
// PacketUtilities::TypeToString()
// ============================================================

std::string PacketUtilities::TypeToString(PacketType type)
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
// PacketUtilities::ExtractHeaderValue()
// ============================================================

std::string PacketUtilities::ExtractHeaderValue(const std::string& raw,
                                                 const std::string& headerName)
{
    // Build uppercase target key.
    std::string targetUpper = headerName;
    std::transform(targetUpper.begin(), targetUpper.end(), targetUpper.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

    std::istringstream stream(raw);
    std::string line;
    bool firstLine = true;

    while (std::getline(stream, line)) {
        if (firstLine) { firstLine = false; continue; }

        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) break;  // End of headers.

        const auto colonPos = line.find(':');
        if (colonPos == std::string::npos) continue;

        std::string key = line.substr(0, colonPos);
        std::transform(key.begin(), key.end(), key.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

        if (key == targetUpper) {
            std::string value = line.substr(colonPos + 1);
            const auto start = value.find_first_not_of(" \t");
            return (start != std::string::npos) ? value.substr(start) : value;
        }
    }
    return {};
}

// ============================================================
// PacketUtilities::ParseHeaders()
// ============================================================

std::vector<Header> PacketUtilities::ParseHeaders(const std::string& raw)
{
    std::vector<Header> result;
    std::istringstream stream(raw);
    std::string line;
    bool firstLine = true;

    while (std::getline(stream, line)) {
        if (firstLine) { firstLine = false; continue; }

        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) break;  // End of headers.

        const auto colonPos = line.find(':');
        if (colonPos == std::string::npos) continue;

        Header h;
        h.name  = line.substr(0, colonPos);
        std::string val = line.substr(colonPos + 1);
        const auto start = val.find_first_not_of(" \t");
        h.value = (start != std::string::npos) ? val.substr(start) : val;
        result.push_back(std::move(h));
    }
    return result;
}

// ============================================================
// PacketUtilities::FormatTimestamp(tp)
// ============================================================

std::string PacketUtilities::FormatTimestamp(
    const std::chrono::system_clock::time_point& tp)
{
    const auto tt  = std::chrono::system_clock::to_time_t(tp);
    const auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
                         tp.time_since_epoch()) % 1000;

    std::tm tmBuf{};
#ifdef _WIN32
    localtime_s(&tmBuf, &tt);
#else
    localtime_r(&tt, &tmBuf);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%Y-%m-%d %H:%M:%S")
        << '.' << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}

// ============================================================
// PacketUtilities::FormatTimestamp() — current time
// ============================================================

std::string PacketUtilities::FormatTimestamp()
{
    return FormatTimestamp(std::chrono::system_clock::now());
}

// ============================================================
// PacketUtilities::FormatElapsed()
// ============================================================

std::string PacketUtilities::FormatElapsed(long long ms)
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
