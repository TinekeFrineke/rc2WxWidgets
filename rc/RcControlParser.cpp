
#include "RcControlParser.h"

#include <iostream>
#include <string>

#include "utilities/strutils.h"

#include "ParserUtilities.h"
#include "RcModel.h"


RcControlParser::RcControlParser(RcDialog& targetDialog)
	: m_targetDialog(targetDialog)
{
}

void RcControlParser::Parse(std::istream& input)
{
    std::string raw;
    while (std::getline(input, raw)) {
        std::string line = trim(stripLineComment(raw));
        if (line.empty()) continue;

        if (line == "END") {
            return;
        }
        if (auto c = parseControlLine(line)) {
            m_targetDialog.controls.push_back(std::move(*c));
        }
    }
}

std::optional<RcControl> RcControlParser::parseControlLine(const std::string& line)
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
    if (kw == "LTEXT") c.type = RcControlType::LText;
    else if (kw == "CTEXT") c.type = RcControlType::CText;
    else if (kw == "RTEXT") c.type = RcControlType::RText;
    else if (kw == "GROUPBOX") c.type = RcControlType::GroupBox;
    else if (kw == "EDITTEXT") c.type = RcControlType::EditText;
    else if (kw == "COMBOBOX") c.type = RcControlType::ComboBox;
    else if (kw == "PUSHBUTTON") c.type = RcControlType::PushButton;
    else if (kw == "DEFPUSHBUTTON") c.type = RcControlType::DefPushButton;
    else if (kw == "ICON") c.type = RcControlType::Icon;
    else if (kw == "CONTROL") c.type = RcControlType::Control;
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

    switch (c.type) {
        case RcControlType::EditText: {
            if (args.size() < 5) return std::nullopt;
            c.id = trim(args[0]);
            c.rectDU = { Str::ToInt(args[1]), Str::ToInt(args[2]), Str::ToInt(args[3]), Str::ToInt(args[4]) };
            if (args.size() > 5) c.style = joinTail(5);
            return c;
        }

        case RcControlType::ComboBox: {
            // COMBOBOX id,x,y,w,h[,style...]
            if (args.size() < 5) return std::nullopt;
            c.id = trim(args[0]);
            c.rectDU = { Str::ToInt(args[1]), Str::ToInt(args[2]), Str::ToInt(args[3]), Str::ToInt(args[4]) };
            if (args.size() > 5) c.style = joinTail(5);
            return c;
        }

        case RcControlType::Control: {
            if (args.size() < 8) return std::nullopt;
            c.text = unquote(args[0]);
            c.id = trim(args[1]);
            c.winClass = unquote(args[2]);
            c.style = trim(args[3]);
            c.rectDU = { Str::ToInt(args[4]), Str::ToInt(args[5]), Str::ToInt(args[6]), Str::ToInt(args[7]) };
            if (args.size() > 8) {
                // rare: exstyle/extra tokens; keep them around.
                c.style += " " + joinTail(8);
            }
            return c;
        }

        case RcControlType::Icon: {
            if (args.size() < 6) return std::nullopt;
            c.id = trim(args[0]); // icon resource id
            c.text = trim(args[1]); // usually IDC_STATIC
            c.rectDU = { Str::ToInt(args[2]), Str::ToInt(args[3]), Str::ToInt(args[4]), Str::ToInt(args[5]) };
            if (args.size() > 6) c.style = joinTail(6);
            return c;
        }
    }

    // Common: LTEXT/GROUPBOX/PUSHBUTTON/etc: "txt",id,x,y,w,h[,style...]
    if (args.size() < 6) return std::nullopt;
    c.text = unquote(args[0]);
    c.id = trim(args[1]);
    c.rectDU = { Str::ToInt(args[2]), Str::ToInt(args[3]), Str::ToInt(args[4]), Str::ToInt(args[5]) };
    if (args.size() > 6) c.style = joinTail(6);
    return c;
}

