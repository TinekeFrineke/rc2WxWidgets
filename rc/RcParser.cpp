#include "RcParser.h"

#include <algorithm>
#include <cctype>
#include <optional>
#include <sstream>
#include <stdexcept>

#include "utilities/strutils.h"

static std::string trim(std::string s)
{
    auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
    while (!s.empty() && isSpace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while (!s.empty() && isSpace(static_cast<unsigned char>(s.back()))) s.pop_back();
    return s;
}

static bool startsWith(const std::string& s, const char* prefix)
{
    const auto n = std::char_traits<char>::length(prefix);
    return s.size() >= n && s.compare(0, n, prefix) == 0;
}

static std::string stripLineComment(const std::string& line)
{
    // RC files often use '//' comments.
    const auto p = line.find("//");
    return (p == std::string::npos) ? line : line.substr(0, p);
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

static std::vector<std::string> splitCsvRespectQuotes(const std::string& s)
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

static int toInt(const std::string& s)
{
    std::string t = trim(s);
    if (t.empty()) return 0;
    // RC commonly uses decimal here; be tolerant.
    return std::stoi(t, nullptr, 0);
}

static std::string unquote(std::string s)
{
    s = trim(s);
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        s.erase(s.begin());
        s.pop_back();
    }
    return s;
}

static std::optional<RcControl> parseControlLine(const std::string& line)
{
    auto l = trim(line);
    if (l.empty()) return std::nullopt;

    auto takeWord = [&](size_t& i) -> std::string {
        while (i < l.size() && std::isspace(static_cast<unsigned char>(l[i]))) ++i;
        size_t start = i;
        while (i < l.size() && !std::isspace(static_cast<unsigned char>(l[i]))) ++i;
        return l.substr(start, i - start);
    };

    size_t i = 0;
    const std::string kw = takeWord(i);
    if (kw.empty()) return std::nullopt;

    const std::string rest = trim(l.substr(i));
    const auto args = splitCsvRespectQuotes(rest);

    RcControl c;
    if (kw == "LTEXT") c.kind = RcControlKind::LText;
    else if (kw == "CTEXT") c.kind = RcControlKind::CText;
    else if (kw == "RTEXT") c.kind = RcControlKind::RText;
    else if (kw == "GROUPBOX") c.kind = RcControlKind::GroupBox;
    else if (kw == "EDITTEXT") c.kind = RcControlKind::EditText;
    else if (kw == "COMBOBOX") c.kind = RcControlKind::ComboBox;
    else if (kw == "PUSHBUTTON") c.kind = RcControlKind::PushButton;
    else if (kw == "DEFPUSHBUTTON") c.kind = RcControlKind::DefPushButton;
    else if (kw == "ICON") c.kind = RcControlKind::Icon;
    else if (kw == "CONTROL") c.kind = RcControlKind::Control;
    else return std::nullopt;

    // The file you showed sticks to a small set of shapes. Support those first.
    // LTEXT "txt",id,x,y,w,h[,style...]
    // EDITTEXT id,x,y,w,h[,style...]
    // CONTROL "txt",id,"class",style,x,y,w,h
    // ICON id,IDC_STATIC,x,y,w,h

    auto joinTail = [&](size_t startIdx) -> std::string {
        std::ostringstream oss;
        for (size_t k = startIdx; k < args.size(); ++k) {
            if (k != startIdx) oss << ", ";
            oss << args[k];
        }
        return trim(oss.str());
    };

    if (c.kind == RcControlKind::EditText) {
        if (args.size() < 5) return std::nullopt;
        c.id = trim(args[0]);
        c.rectDU = { toInt(args[1]), toInt(args[2]), toInt(args[3]), toInt(args[4]) };
        if (args.size() > 5) c.style = joinTail(5);
        return c;
    }

    if (c.kind == RcControlKind::Control) {
        if (args.size() < 8) return std::nullopt;
        c.text = unquote(args[0]);
        c.id = trim(args[1]);
        c.winClass = unquote(args[2]);
        c.style = trim(args[3]);
        c.rectDU = { toInt(args[4]), toInt(args[5]), toInt(args[6]), toInt(args[7]) };
        if (args.size() > 8) {
            // rare: exstyle/extra tokens; keep them around.
            c.style += " " + joinTail(8);
        }
        return c;
    }

    if (c.kind == RcControlKind::Icon) {
        if (args.size() < 6) return std::nullopt;
        c.id = trim(args[0]); // icon resource id
        c.text = trim(args[1]); // usually IDC_STATIC
        c.rectDU = { toInt(args[2]), toInt(args[3]), toInt(args[4]), toInt(args[5]) };
        if (args.size() > 6) c.style = joinTail(6);
        return c;
    }

    // Common: LTEXT/GROUPBOX/PUSHBUTTON/etc: "txt",id,x,y,w,h[,style...]
    if (args.size() < 6) return std::nullopt;
    c.text = unquote(args[0]);
    c.id = trim(args[1]);
    c.rectDU = { toInt(args[2]), toInt(args[3]), toInt(args[4]), toInt(args[5]) };
    if (args.size() > 6) c.style = joinTail(6);
    return c;
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

        if (!inBegin) {
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
                inBegin = true;
                continue;
            }
            // Ignore other lines (LANGUAGE, GUIDELINES, etc) until BEGIN.
            continue;
        }

        if (line == "END") {
            curDlg = nullptr;
            inBegin = false;
            continue;
        }

        if (auto c = parseControlLine(line)) {
            curDlg->controls.push_back(std::move(*c));
        }
    }

    return out;
}

