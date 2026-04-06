#include "TextFile.h"

#include <cstdint>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

static std::vector<std::uint8_t> readAllBytes(const std::string& path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f) throw std::runtime_error("Could not open input file.");
    return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

static void appendUtf8(std::string& out, std::uint32_t cp)
{
    if (cp <= 0x7F) {
        out.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7FF) {
        out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0xFFFF) {
        out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp <= 0x10FFFF) {
        out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
        // replacement char
        appendUtf8(out, 0xFFFD);
    }
}

static std::uint16_t readU16LE(const std::uint8_t* p)
{
    return static_cast<std::uint16_t>(p[0] | (static_cast<std::uint16_t>(p[1]) << 8));
}

static std::uint16_t readU16BE(const std::uint8_t* p)
{
    return static_cast<std::uint16_t>(p[1] | (static_cast<std::uint16_t>(p[0]) << 8));
}

static std::string utf16ToUtf8(const std::uint8_t* data, size_t bytes, bool bigEndian)
{
    std::string out;
    out.reserve(bytes); // upper bound-ish

    auto readU16 = bigEndian ? readU16BE : readU16LE;

    size_t i = 0;
    while (i + 1 < bytes) {
        const std::uint16_t u = readU16(data + i);
        i += 2;

        // surrogate pair?
        if (u >= 0xD800 && u <= 0xDBFF) {
            if (i + 1 >= bytes) {
                appendUtf8(out, 0xFFFD);
                break;
            }
            const std::uint16_t lo = readU16(data + i);
            i += 2;
            if (lo >= 0xDC00 && lo <= 0xDFFF) {
                const std::uint32_t hi10 = static_cast<std::uint32_t>(u - 0xD800);
                const std::uint32_t lo10 = static_cast<std::uint32_t>(lo - 0xDC00);
                const std::uint32_t cp = 0x10000u + ((hi10 << 10) | lo10);
                appendUtf8(out, cp);
            } else {
                appendUtf8(out, 0xFFFD);
            }
            continue;
        }

        if (u >= 0xDC00 && u <= 0xDFFF) {
            appendUtf8(out, 0xFFFD);
            continue;
        }

        appendUtf8(out, u);
    }

    return out;
}

std::string readFileAsUtf8(const std::string& path)
{
    const auto bytes = readAllBytes(path);
    if (bytes.empty()) return {};

    // UTF-8 BOM: EF BB BF
    if (bytes.size() >= 3 && bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF) {
        return std::string(reinterpret_cast<const char*>(bytes.data() + 3), bytes.size() - 3);
    }

    // UTF-16 LE BOM: FF FE
    if (bytes.size() >= 2 && bytes[0] == 0xFF && bytes[1] == 0xFE) {
        return utf16ToUtf8(bytes.data() + 2, bytes.size() - 2, /*bigEndian*/ false);
    }

    // UTF-16 BE BOM: FE FF
    if (bytes.size() >= 2 && bytes[0] == 0xFE && bytes[1] == 0xFF) {
        return utf16ToUtf8(bytes.data() + 2, bytes.size() - 2, /*bigEndian*/ true);
    }

    // No BOM: assume already UTF-8 or ANSI.
    return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

