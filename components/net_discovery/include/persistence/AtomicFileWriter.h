/**
 * @file AtomicFileWriter.h
 * @brief Helper utility for safe POSIX atomic file writes (Write .tmp -> fflush -> fsync -> close -> rename).
 */

#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace NetDiscovery {
namespace Persistence {

class AtomicFileWriter {
public:
    /**
     * @brief Writes text data atomically to targetFilePath.
     * @param targetFilePath Final destination path.
     * @param content String data to write.
     * @return true if successful, false otherwise.
     */
    static bool WriteStringAtomic(const std::string& targetFilePath, const std::string& content);

    /**
     * @brief Writes binary data atomically to targetFilePath.
     * @param targetFilePath Final destination path.
     * @param data Raw binary vector to write.
     * @return true if successful, false otherwise.
     */
    static bool WriteBinaryAtomic(const std::string& targetFilePath, const std::vector<uint8_t>& data);

    /**
     * @brief Ensures all parent directories for a path exist recursively.
     * @param path File or directory path.
     * @return true if directory structure exists or was created.
     */
    static bool EnsureDirectoryExists(const std::string& path);
};

} // namespace Persistence
} // namespace NetDiscovery
