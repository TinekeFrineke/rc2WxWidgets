#pragma once

#include <string>

// Reads a file as UTF-8 text.
//
// Supported inputs:
// - UTF-8 with or without BOM
// - UTF-16LE with BOM
// - UTF-16BE with BOM
//
// If there's no BOM, bytes are returned as-is (assumed UTF-8 or ANSI).
// Throws std::runtime_error on read errors or unsupported encodings.
std::string readFileAsUtf8(const std::string& path);

