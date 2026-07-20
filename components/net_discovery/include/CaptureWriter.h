/**
 * @file CaptureWriter.h
 * @brief Writes captured Packets to the filesystem for offline analysis.
 *
 * CaptureWriter manages a session directory structure under a configurable
 * base path and writes packets to sequentially-numbered, non-overwriting files.
 *
 * Directory structure:
 *   <basePath>/
 *     active/
 *       001_ssdp_all_192.168.x.y.txt
 *       002_upnp_rootdevice_192.168.x.y.txt
 *       ...
 *     passive/
 *       001_notify_192.168.x.y.txt
 *       ...
 *
 * Refactored in architectural stabilization:
 *   - SaveActiveResponse(ST, SSDPResponse) removed.
 *   - SavePassivePacket(ip, raw) removed.
 *   - Both replaced by Packet-based equivalents (see below).
 *
 * Platform notes:
 *   - Uses std::filesystem (C++17) for path manipulation.
 *   - No WinSock2 dependency.
 *
 * Portability: std::filesystem only — portable to ESP-IDF 5+
 *              (requires CONFIG_NEWLIB_NANO_FORMAT=n for full float
 *               support; filesystem itself is available via SPIFFS/LittleFS).
 */

#pragma once

#include "core/Packet.h"

#include <cstdint>
#include <string>
#include <string>

namespace NetDiscovery {

// ============================================================
// CaptureWriter
// ============================================================

class CaptureWriter {
public:
    /**
     * @brief Construct a CaptureWriter for the given base directory.
     *
     * Creates the base directory and subdirectories (active/, passive/)
     * if they do not already exist.
     *
     * @param basePath  Root directory for capture files
     *                  (relative or absolute; relative paths are
     *                   resolved against the current working directory).
     * @throws std::filesystem::filesystem_error on directory creation failure.
     */
    explicit CaptureWriter(const std::string& basePath);

    ~CaptureWriter() = default;

    CaptureWriter(const CaptureWriter&) = delete;
    CaptureWriter& operator=(const CaptureWriter&) = delete;
    CaptureWriter(CaptureWriter&&) noexcept = default;
    CaptureWriter& operator=(CaptureWriter&&) noexcept = default;

    // ----------------------------------------------------------------
    // Active discovery
    // ----------------------------------------------------------------

    /**
     * @brief Persist a Packet received during active M-SEARCH discovery.
     *
     * Writes the packet's metadata (timestamp, transport, source, ST)
     * and raw payload to:
     *   <basePath>/active/<NNN>_<sanitized-ST>_<sender-IP>.txt
     *
     * The sequence number NNN is monotonically increasing and never
     * reuses a filename in the current session.
     *
     * @param searchTarget  The ST value used for the M-SEARCH (for the filename).
     * @param packet        The received Packet to save.
     */
    void SaveActivePacket(const std::string& searchTarget, const Packet& packet);

    // ----------------------------------------------------------------
    // Passive listening
    // ----------------------------------------------------------------

    /**
     * @brief Persist a Packet received during passive multicast listening.
     *
     * Writes to:
     *   <basePath>/passive/<NNN>_<type>_<sender-IP>.txt
     *
     * @param packet  The received multicast Packet.
     */
    void SavePassivePacket(const Packet& packet);

    // ----------------------------------------------------------------
    // Session info
    // ----------------------------------------------------------------

    /**
     * @brief Return the resolved absolute path to the session directory.
     */
    const std::string& BasePath() const noexcept;

    /**
     * @brief Return the total number of files written in this session.
     */
    uint32_t FileCount() const noexcept;

private:
    // ---- Internal helpers ----
    std::string BuildFilename(const std::string& dir,
                              const std::string& prefix,
                              const std::string& suffix) const;

    static std::string SanitizeForFilename(const std::string& raw);

    void WritePacketFile(const std::string& filepath,
                         const std::string& header,
                         const Packet& packet) const;

    // ---- State ----
    std::string m_basePath;
    std::string m_activeDir;
    std::string m_passiveDir;

    mutable uint32_t m_fileCount{0};        ///< Session-level file counter.
    mutable uint32_t m_activeCount{0};      ///< Counter for active/ filenames.
    mutable uint32_t m_passiveCount{0};     ///< Counter for passive/ filenames.
};

} // namespace NetDiscovery
