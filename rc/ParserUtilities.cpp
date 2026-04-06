
#include "ParserUtilities.h"


std::string unquote(std::string s)
{
    s = trim(s);
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        s.erase(s.begin());
        s.pop_back();
    }
    return s;
}

std::string trim(std::string s)
{
    auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
    while (!s.empty() && isSpace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while (!s.empty() && isSpace(static_cast<unsigned char>(s.back()))) s.pop_back();
    return s;
}

std::vector<std::string> splitCsvRespectQuotes(const std::string& s)
{
    std::vector<std::string> out;
    std::string cur;
    bool inQuotes = false;
    for (size_t i = 0; i < s.size(); ++i) {
        const char c = s[i];
        if (c == '"') {
            inQuotes = !inQuotes;
            cur.push_back(c);
            continue;
        }
        if (!inQuotes && c == ',') {
            out.push_back(trim(cur));
            cur.clear();
            continue;
        }
        cur.push_back(c);
    }
    if (!cur.empty()) out.push_back(trim(cur));
    return out;
}

std::string stripLineComment(const std::string& line)
{
    // RC files often use '//' comments.
    const auto p = line.find("//");
    return (p == std::string::npos) ? line : line.substr(0, p);
}
