#include "RcParser.h"

#include <algorithm>
#include <cctype>
#include <optional>
#include <sstream>
#include <stdexcept>

#include "utilities/strutils.h"

namespace wxConvert {
namespace {

std::string trim(std::string s)
{
    auto isSpace = [] (unsigned char c) { return std::isspace(c) != 0; };
    while (!s.empty() && isSpace(static_cast<unsigned char>(s.front()))) s.erase(s.begin());
    while (!s.empty() && isSpace(static_cast<unsigned char>(s.back()))) s.pop_back();
    return s;
}

std::string trim(std::string s, char c)
{
    while (!s.empty() && s.front() == c) s.erase(s.begin());
    while (!s.empty() && s.back() == c) s.pop_back();
    return s;
}

bool startsWith(const std::string& s, const char* prefix)
{
    const auto n = std::char_traits<char>::length(prefix);
    return s.size() >= n && s.compare(0, n, prefix) == 0;
}

std::string stripLineComment(const std::string& line)
{
    // RC files often use '//' comments.
    const auto p = line.find("//");
    return (p == std::string::npos) ? line : line.substr(0, p);
}

std::optional<std::string> parseQuoted(const std::string& s, size_t& i)
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
        }
        else {
            out.push_back(c);
        }
    }
    return out;
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

int toInt(const std::string& s)
{
    std::string t = trim(s);
    if (t.empty()) return 0;
    // RC commonly uses decimal here; be tolerant.
    return std::stoi(t, nullptr, 0);
}

std::string unquote(std::string s)
{
    s = trim(s);
    s = trim(s, '"');
    return s;
}

std::optional<RcControl> parseControlLine(const std::string& line)
{
    auto l = trim(line);
    if (l.empty()) return std::nullopt;

    auto takeWord = [&] (size_t& i) -> std::string {
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
    if (kw == "LTEXT") c.kind = RcControl::Type::LText;
    else if (kw == "CTEXT") c.kind = RcControl::Type::CText;
    else if (kw == "RTEXT") c.kind = RcControl::Type::RText;
    else if (kw == "GROUPBOX") c.kind = RcControl::Type::GroupBox;
    else if (kw == "EDITTEXT") c.kind = RcControl::Type::EditText;
    else if (kw == "COMBOBOX") c.kind = RcControl::Type::ComboBox;
    else if (kw == "PUSHBUTTON") c.kind = RcControl::Type::PushButton;
    else if (kw == "DEFPUSHBUTTON") c.kind = RcControl::Type::DefPushButton;
    else if (kw == "ICON") c.kind = RcControl::Type::Icon;
    else if (kw == "CONTROL") c.kind = RcControl::Type::Control;
    else return std::nullopt;

    // The file you showed sticks to a small set of shapes. Support those first.
    // LTEXT "txt",id,x,y,w,h[,style...]
    // EDITTEXT id,x,y,w,h[,style...]
    // CONTROL "txt",id,"class",style,x,y,w,h
    // ICON id,IDC_STATIC,x,y,w,h

    auto joinTail = [&] (size_t startIdx) -> std::string {
        std::ostringstream oss;
        for (size_t k = startIdx; k < args.size(); ++k) {
            if (k != startIdx) oss << ", ";
            oss << args[k];
        }
        return trim(oss.str());
        };

    if (c.kind == RcControl::Type::EditText) {
        if (args.size() < 5) return std::nullopt;
        c.id = trim(args[0]);
        c.rectDU = { toInt(args[1]), toInt(args[2]), toInt(args[3]), toInt(args[4]) };
        if (args.size() > 5) c.style = joinTail(5);
        return c;
    }

    if (c.kind == RcControl::Type::ComboBox) {
        // COMBOBOX id,x,y,w,h[,style...]
        if (args.size() < 5) return std::nullopt;
        c.id = trim(args[0]);
        c.rectDU = { toInt(args[1]), toInt(args[2]), toInt(args[3]), toInt(args[4]) };
        if (args.size() > 5) c.style = joinTail(5);
        return c;
    }

    if (c.kind == RcControl::Type::Control) {
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

    if (c.kind == RcControl::Type::Icon) {
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

bool isBegin(const std::string line)
{
    return trim(line) == "BEGIN";
}

bool isEnd(const std::string line)
{
    return trim(line) == "END";
}

std::istream& getLine(std::istream& input, std::string& line)
{
    std::getline(input, line);
    line = trim(line);
    line = trim(line, '\r');
    return input;
}

void parseDialogContent(std::istream& input, RcDialog& dialog)
{
    std::string line;
    while (getLine(input, line) && !isEnd(line)) {
        std::cout << "parseDialogContent() line " << line << '\n';
        auto control = parseControlLine(line);
        if (control)
            dialog.controls.push_back(*control);
    }
}

namespace {
std::string camelCasualize(const std::string& string)
{
    if (string.empty())
        throw std::invalid_argument("camelCasualize: string is empty");
    auto newName = Str::ToLower(string);
    newName[0] = std::toupper(newName[0]);
    return newName;
}
std::string beautifyDialogName(const std::string& resourceId)
{
    if (resourceId.empty())
        throw std::invalid_argument("beautifyDialogName: Resource ID is empty");
    auto tokens = Str::StrTok(resourceId, "_");
    if (tokens.size() < 2)
        throw std::invalid_argument("Resource ID " + resourceId + " is invalid");
    tokens.erase(tokens.begin());

    std::string name;
    for (const auto& token : tokens)
        name += camelCasualize(token);

    return name;
}
} // namespace

std::unique_ptr<RcDialog> createDialogFromTokens(const std::vector<std::string>& tokens, std::istream& input)
{
    if (tokens.size() < 6)
        throw std::invalid_argument("Line does not have enough tokens");

    auto dialog = std::make_unique<RcDialog>();
    dialog->name = beautifyDialogName(tokens[0]);
    dialog->rectDU = { toInt(tokens[2]), toInt(tokens[3]), toInt(tokens[4]), toInt(tokens[5]) };

    std::string line;
    while (getLine(input, line)/* && !isEnd(line)*/) {
        std::cout << "createDialogFromTokens() line " << line << '\n';
        if (startsWith(line, "STYLE")) {
            auto p = line.find(' ');
            dialog->style = trim(p == std::string::npos ? "" : line.substr(p + 1));
            continue;
        }
        if (startsWith(line, "EXSTYLE")) {
            auto p = line.find(' ');
            dialog->exStyle = trim(p == std::string::npos ? "" : line.substr(p + 1));
            continue;
        }
        if (startsWith(line, "CAPTION")) {
            size_t i = std::string("CAPTION").size();
            auto q = parseQuoted(line, i);
            dialog->caption = q ? *q : "";
            continue;
        }
        if (startsWith(line, "FONT")) {
            // FONT 8, "MS Shell Dlg", ...
            const auto args = splitCsvRespectQuotes(trim(line.substr(4)));
            if (!args.empty()) dialog->fontPointSize = toInt(args[0]);
            if (args.size() >= 2) dialog->fontFace = unquote(args[1]);
            continue;
        }
        if (isBegin(line)) {
            parseDialogContent(input, *dialog);
            return dialog;;
        }

        throw std::domain_error("Invalid type: " + line);
    }

    return dialog;
}

}

RcFile RcParser::parse(std::istream& in) const
{
    RcFile out;

    std::unique_ptr<RcDialog> dialog = parseDialog(in);
    while (dialog != nullptr) {
        out.dialogs.push_back(*dialog);
        dialog = parseDialog(in);
    }

    return out;
}

std::unique_ptr<RcDialog> RcParser::parseDialog(std::istream& in) const
{
    std::string raw;
    std::unique_ptr<RcDialog> curDlg;

    while (getLine(in, raw)) {
        std::cout << "RcParser::parseDialog() line " << raw << '\n';
        std::string line = trim(stripLineComment(raw));
        if (line.empty()) continue;

        // Start of dialog:
        // IDD_SOMETHING DIALOGEX 0, 0, 170, 62
        const auto tokens = Str::StrTok(line, " ");
        if (tokens.size() >= 6 && (tokens[1] == "DIALOGEX" || tokens[1] == "DIALOG")) {
            curDlg = createDialogFromTokens(tokens, in);
            return curDlg;
        }
    }

    return curDlg;
}

} // namespace wxConvert