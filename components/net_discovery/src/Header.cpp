/**
 * @file Header.cpp
 * @brief Implementation of free functions for Header collections.
 *
 * Core layer — no protocol-specific dependencies.
 *
 * Portability: STL only — fully portable to ESP-IDF.
 */

#include "core/Header.h"

#include <algorithm>
#include <cctype>

namespace NetDiscovery {

// ============================================================
// Internal helper — case-insensitive string equality
// ============================================================

namespace {



bool Iequals(const std::string& a, const std::string& b) noexcept
{
    if (a.size() != b.size()) return false;
    return std::equal(a.begin(), a.end(), b.begin(),
        [](unsigned char x, unsigned char y) {
            return std::tolower(x) == std::tolower(y);
        });
}

} // anonymous namespace

// ============================================================
// FindHeader()
// ============================================================

const Header* FindHeader(const std::vector<Header>& headers,
                         const std::string&          name) noexcept
{
    for (const auto& h : headers) {
        if (Iequals(h.name, name)) {
            return &h;
        }
    }
    return nullptr;
}

// ============================================================
// GetHeaderValue()
// ============================================================

std::string GetHeaderValue(const std::vector<Header>& headers,
                            const std::string&          name) noexcept
{
    const Header* h = FindHeader(headers, name);
    return (h != nullptr) ? h->value : std::string{};
}

} // namespace NetDiscovery
