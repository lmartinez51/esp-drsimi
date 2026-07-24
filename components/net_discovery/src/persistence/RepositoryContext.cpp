/**
 * @file RepositoryContext.cpp
 * @brief Implementation of RepositoryContext infrastructure helper.
 */

#include "persistence/RepositoryContext.h"
#include <chrono>

namespace NetDiscovery {
namespace Persistence {

RepositoryContext::RepositoryContext(std::string basePath)
    : m_basePath(std::move(basePath)) {}

int64_t RepositoryContext::GetCurrentTimestamp() const {
    using namespace std::chrono;
    return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
}

} // namespace Persistence
} // namespace NetDiscovery
