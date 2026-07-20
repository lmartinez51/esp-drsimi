/**
 * @file Header.h
 * @brief HTTP-style header name/value pair.
 *
 * Core layer — no protocol-specific dependencies.
 * Part of the NetDiscovery Core domain model.
 *
 * Using a std::vector<Header> instead of std::map preserves:
 *   - Original header ordering (important for HTTP/1.1 compliance).
 *   - Duplicate header names (e.g. multiple Set-Cookie headers).
 *
 * Portability: STL only — fully portable to ESP-IDF.
 */

#pragma once

#include <string>
#include <vector>

namespace NetDiscovery {

// ============================================================
// Header
// ============================================================

/**
 * @brief A single HTTP-style header field.
 *
 * Both name and value are stored as raw strings; no case folding or
 * normalization is applied at the storage level. Comparison is the
 * responsibility of the code performing the lookup.
 */
struct Header {
    /// Header field name (e.g. "LOCATION", "USN", "Cache-Control").
    std::string name;

    /// Header field value, with leading whitespace stripped.
    std::string value;
};

// ============================================================
// Free functions for Header collections
// ============================================================

/**
 * @brief Find the first header whose name matches the given key
 *        using a case-insensitive comparison.
 *
 * @param headers  Collection to search.
 * @param name     Header name to find (case-insensitive).
 * @return Pointer to the matching Header, or nullptr if not found.
 */
const Header* FindHeader(const std::vector<Header>& headers,
                         const std::string&          name) noexcept;

/**
 * @brief Return the value of the first header matching @p name,
 *        or an empty string if not found.
 *
 * @param headers  Collection to search.
 * @param name     Header name to find (case-insensitive).
 * @return Header value string, or "".
 */
std::string GetHeaderValue(const std::vector<Header>& headers,
                            const std::string&          name) noexcept;

} // namespace NetDiscovery
