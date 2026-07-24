/**
 * @file AtomicFileWriter.cpp
 * @brief Helper implementation for safe POSIX atomic file writes.
 */

#include "persistence/AtomicFileWriter.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "esp_log.h"

static const char* TAG = "AtomicFileWriter";

namespace NetDiscovery {
namespace Persistence {

bool AtomicFileWriter::EnsureDirectoryExists(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return true; // Exists
    }

    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos && pos > 0) {
        std::string parent = path.substr(0, pos);
        EnsureDirectoryExists(parent);
    }

    if (mkdir(path.c_str(), 0777) == 0 || errno == EEXIST) {
        return true;
    }

    ESP_LOGE(TAG, "Failed to create directory: %s (errno=%d)", path.c_str(), errno);
    return false;
}

bool AtomicFileWriter::WriteStringAtomic(const std::string& targetFilePath, const std::string& content) {
    std::vector<uint8_t> vec(content.begin(), content.end());
    return WriteBinaryAtomic(targetFilePath, vec);
}

bool AtomicFileWriter::WriteBinaryAtomic(const std::string& targetFilePath, const std::vector<uint8_t>& data) {
    // 1. Ensure target directory exists
    size_t pos = targetFilePath.find_last_of("/\\");
    if (pos != std::string::npos) {
        std::string dir = targetFilePath.substr(0, pos);
        if (!EnsureDirectoryExists(dir)) {
            return false;
        }
    }

    std::string tmpPath = targetFilePath + ".tmp";

    // 2. Open temporary file
    FILE* f = fopen(tmpPath.c_str(), "wb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open temp write file: %s (errno=%d)", tmpPath.c_str(), errno);
        return false;
    }

    // 3. Write payload
    if (!data.empty()) {
        size_t written = fwrite(data.data(), 1, data.size(), f);
        if (written != data.size()) {
            ESP_LOGE(TAG, "Write incomplete for %s: %u/%u bytes", tmpPath.c_str(), (unsigned)written, (unsigned)data.size());
            fclose(f);
            unlink(tmpPath.c_str());
            return false;
        }
    }

    // 4. Flush user-space buffers
    if (fflush(f) != 0) {
        ESP_LOGE(TAG, "fflush failed for %s", tmpPath.c_str());
        fclose(f);
        unlink(tmpPath.c_str());
        return false;
    }

    // 5. Sync to storage controller
    int fd = fileno(f);
    if (fd >= 0) {
        fsync(fd);
    }

    // 6. Close temporary file
    fclose(f);

    // 7. Atomic POSIX rename
    if (rename(tmpPath.c_str(), targetFilePath.c_str()) != 0) {
        ESP_LOGE(TAG, "Atomic rename failed from %s to %s (errno=%d)", tmpPath.c_str(), targetFilePath.c_str(), errno);
        unlink(tmpPath.c_str());
        return false;
    }

    return true;
}

} // namespace Persistence
} // namespace NetDiscovery
