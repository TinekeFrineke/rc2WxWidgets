#include "RcParser.h"

#include <algorithm>
#include <cctype>
#include <optional>
#include <sstream>
#include <stdexcept>

#include "utilities/strutils.h"

#include "ParserUtilities.h"
#include "RcControlParser.h"

static bool startsWith(const std::string& s, const char* prefix)
{
    const auto n = std::char_traits<char>::length(prefix);
    return s.size() >= n && s.compare(0, n, prefix) == 0;
}

static std::optional<std::string> parseQuoted(const std::string& s, size_t& i)
{
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
    if (i >= s.size() || s[i] != '"') return std::nullopt;
    ++i;
    std::string out;
    while (i < s.size()) {
        const char c = s[i++];
        if (c == '"') break;
        if (c == '\\' && i < s.size()) {
            // Minimal escape handling for \" and \\.
            const char n = s[i++];
            out.push_back(n);
        } else {
            out.push_back(c);
        }
    }
    return out;
}

static int toInt(const std::string& s)
{
    std::string t = trim(s);
    if (t.empty()) return 0;
    // RC commonly uses decimal here; be tolerant.
    return std::stoi(t, nullptr, 0);
}

RcFile RcParser::parse(std::istream& in) const
{
    RcFile out;

    std::string raw;
    RcDialog* curDlg = nullptr;
    bool inBegin = false;

    while (std::getline(in, raw)) {
        std::string line = trim(stripLineComment(raw));
        if (line.empty()) continue;

        if (!curDlg) {
            // Start of dialog:
            // IDD_SOMETHING DIALOGEX 0, 0, 170, 62
            const auto toks = Str::StrTok(line, " ");
            if (toks.size() >= 2 && (toks[1] == "DIALOGEX" || toks[1] == "DIALOG")) {
                const std::string& kw = toks[1];
                const auto kwPos = line.find(" " + kw + " ");
                if (kwPos == std::string::npos) continue; // assume spaces

                RcDialog dlg;
                dlg.name = toks[0];
                const std::string tail = trim(line.substr(kwPos + kw.size() + 2));
                const auto args = splitCsvRespectQuotes(tail);
                if (args.size() >= 4) {
                    dlg.rectDU = { toInt(args[0]), toInt(args[1]), toInt(args[2]), toInt(args[3]) };
                }
                out.dialogs.push_back(std::move(dlg));
                curDlg = &out.dialogs.back();
                continue;
            }
            continue;
        }

        if (startsWith(line, "STYLE")) {
            auto p = line.find(' ');
            curDlg->style = trim(p == std::string::npos ? "" : line.substr(p + 1));
            continue;
        }
        if (startsWith(line, "EXSTYLE")) {
            auto p = line.find(' ');
            curDlg->exStyle = trim(p == std::string::npos ? "" : line.substr(p + 1));
            continue;
        }
        if (startsWith(line, "CAPTION")) {
            size_t i = std::string("CAPTION").size();
            auto q = parseQuoted(line, i);
            curDlg->caption = q ? *q : "";
            continue;
        }
        if (startsWith(line, "FONT")) {
            // FONT 8, "MS Shell Dlg", ...
            const auto args = splitCsvRespectQuotes(trim(line.substr(4)));
            if (!args.empty()) curDlg->fontPointSize = toInt(args[0]);
            if (args.size() >= 2) curDlg->fontFace = unquote(args[1]);
            continue;
        }
        if (line == "BEGIN") {
            RcControlParser parser(*curDlg);
            parser.Parse(in);
            out.dialogs.push_back(*curDlg);
            curDlg = nullptr;
        }
    }

    return out;
}

