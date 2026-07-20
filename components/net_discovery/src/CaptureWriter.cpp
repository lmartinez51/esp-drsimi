/**
 * @file CaptureWriter.cpp
 * @brief CaptureWriter implementation — writes Packets to the filesystem.
 */

#include "../include/CaptureWriter.h"
#include "../include/PacketUtilities.h"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include "esp_log.h"

static const char* TAG = "NetDiscovery";

namespace NetDiscovery {

// ============================================================
// Constructor
// ============================================================

CaptureWriter::CaptureWriter(const std::string& basePath)
    : m_basePath(basePath)
    , m_activeDir(basePath + "/active")
    , m_passiveDir(basePath + "/passive")
{
    auto ensure_dir = [](const std::string& path) {
        std::string::size_type pos = 0;
        do {
            pos = path.find_first_of("\\/", pos + 1);
            std::string sub = path.substr(0, pos);
            struct stat st;
            if (stat(sub.c_str(), &st) != 0) {
                mkdir(sub.c_str(), 0777);
            }
        } while (pos != std::string::npos);
        struct stat st;
        if (stat(path.c_str(), &st) != 0) {
            mkdir(path.c_str(), 0777);
        }
    };
    ensure_dir(m_activeDir);
    ensure_dir(m_passiveDir);
}

// ============================================================
// SaveActivePacket()
// ============================================================

void CaptureWriter::SaveActivePacket(const std::string& searchTarget,
                                      const Packet&       packet)
{
    ++m_activeCount;
    ++m_fileCount;

    const std::string senderSafe  = SanitizeForFilename(packet.source.address);
    const std::string targetSafe  = SanitizeForFilename(searchTarget);
    const std::string prefix      = targetSafe + "_" + senderSafe;

    const std::string filepath = BuildFilename(m_activeDir, prefix, ".txt");

    // Build a human-readable metadata header for the file.
    std::ostringstream meta;
    meta << "Timestamp : " << PacketUtilities::FormatTimestamp(packet.timestamp) << "\n"
         << "ST        : " << searchTarget                                        << "\n"
         << "Source    : " << packet.source.address << ":" << packet.source.port << "\n"
         << "Dest      : " << packet.destination.address << ":" << packet.destination.port << "\n"
         << "Transport : " << ToString(packet.transport)                          << "\n"
         << "Protocol  : " << ToString(packet.protocol)                           << "\n";

    WritePacketFile(filepath, meta.str(), packet);
}

// ============================================================
// SavePassivePacket()
// ============================================================

void CaptureWriter::SavePassivePacket(const Packet& packet)
{
    ++m_passiveCount;
    ++m_fileCount;

    const PacketType type = PacketUtilities::DetectType(packet.rawPayload);
    const std::string typeSafe   = SanitizeForFilename(PacketUtilities::TypeToString(type));
    const std::string senderSafe = SanitizeForFilename(packet.source.address);
    const std::string prefix     = typeSafe + "_" + senderSafe;

    const std::string filepath = BuildFilename(m_passiveDir, prefix, ".txt");

    std::ostringstream meta;
    meta << "Timestamp : " << PacketUtilities::FormatTimestamp(packet.timestamp) << "\n"
         << "Type      : " << PacketUtilities::TypeToString(type)                 << "\n"
         << "Source    : " << packet.source.address << ":" << packet.source.port << "\n"
         << "Transport : " << ToString(packet.transport)                          << "\n"
         << "Protocol  : " << ToString(packet.protocol)                           << "\n";

    WritePacketFile(filepath, meta.str(), packet);
}

// ============================================================
// BasePath()
// ============================================================

const std::string& CaptureWriter::BasePath() const noexcept
{
    return m_basePath;
}

// ============================================================
// FileCount()
// ============================================================

uint32_t CaptureWriter::FileCount() const noexcept
{
    return m_fileCount;
}

// ============================================================
// BuildFilename()
// ============================================================

std::string CaptureWriter::BuildFilename(
    const std::string& dir,
    const std::string& prefix,
    const std::string& suffix) const
{
    // Use the combined active+passive sequence so numbers never repeat
    // within a session, making it easy to correlate timestamps.
    std::ostringstream name;
    name << std::setw(3) << std::setfill('0') << m_fileCount
         << "_" << prefix << suffix;

    std::string candidate = dir + "/" + name.str();

    // Guard against extremely rare collisions (e.g. rapid same-IP responses).
    int disambig = 0;
    auto exists = [](const std::string& p) {
        struct stat st;
        return (stat(p.c_str(), &st) == 0);
    };
    
    while (exists(candidate)) {
        ++disambig;
        std::ostringstream retry;
        retry << std::setw(3) << std::setfill('0') << m_fileCount
              << "_" << prefix << "_" << disambig << suffix;
        candidate = dir + "/" + retry.str();
    }
    return candidate;
}

// ============================================================
// SanitizeForFilename()
// ============================================================

std::string CaptureWriter::SanitizeForFilename(const std::string& raw)
{
    std::string result;
    result.reserve(raw.size());
    for (const char c : raw) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '-') {
            result += c;
        } else {
            result += '_';
        }
    }
    // Collapse repeated underscores.
    std::string collapsed;
    collapsed.reserve(result.size());
    bool prevUnderscore = false;
    for (const char c : result) {
        if (c == '_' && prevUnderscore) continue;
        collapsed += c;
        prevUnderscore = (c == '_');
    }
    return collapsed;
}

// ============================================================
// WritePacketFile()
// ============================================================

void CaptureWriter::WritePacketFile(const std::string& filepath,
                                     const std::string& header,
                                     const Packet& packet) const
{
    std::ofstream file(filepath, std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        ESP_LOGE(TAG, "CaptureWriter: failed to open: %s", filepath.c_str());
        return;
    }

    file << "========================================\n"
         << header
         << "========================================\n\n"
         << packet.rawPayload
         << "\n";
}

} // namespace NetDiscovery
