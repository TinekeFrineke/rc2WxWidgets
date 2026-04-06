
#pragma once

#include <string>
#include <vector>

std::vector<std::string> splitCsvRespectQuotes(const std::string& s);
std::string stripLineComment(const std::string& line);
std::string trim(std::string s);
std::string unquote(std::string s);
