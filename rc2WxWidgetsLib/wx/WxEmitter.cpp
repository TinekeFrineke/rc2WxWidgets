#include "WxEmitter.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include "CodeBuilder.h"

namespace wxConvert {

namespace {

const std::string idExpr = "wxID_ANY"; // safer default; user can map later
CodeBuilder& createControl(const Control& ctl, const std::string& parent, CodeBuilder& c);

std::string sanitizeIdent(std::string s)
{
    // Turn RC ids (IDD_FOO) into a safe C++ identifier suffix.
    for (auto& ch : s) {
        if (!std::isalnum(static_cast<unsigned char>(ch))) ch = '_';
    }
    if (!s.empty() && std::isdigit(static_cast<unsigned char>(s[0]))) s = "_" + s;
    return s;
}

std::string cppStringLiteral(const std::string& s)
{
    std::ostringstream oss;
    oss << "\"";
    for (char c : s) {
        switch (c) {
            case '\\': oss << "\\\\"; break;
            case '"': oss << "\\\""; break;
            case '\n': oss << "\\n"; break;
            case '\r': oss << "\\r"; break;
            case '\t': oss << "\\t"; break;
            default: oss << c; break;
        }
    }
    oss << "\"";
    return oss.str();
}

bool containsToken(const std::string& haystack, const char* needle)
{
    return haystack.find(needle) != std::string::npos;
}

std::string wxTextCtrlStyleFromRc(const std::string& rcStyle)
{
    std::string out;
    if (containsToken(rcStyle, "ES_READONLY")) out += " | wxTE_READONLY";
    if (containsToken(rcStyle, "ES_MULTILINE")) out += " | wxTE_MULTILINE";
    // ES_AUTOHSCROLL is default-ish; skip.
    if (out.empty()) return "0";
    return out.substr(3); // drop leading " | "
}

std::string wxListCtrlStyleFromRc(const std::string& rcStyle)
{
    // Minimal mapping for the sample: LVS_REPORT -> wxLC_REPORT
    std::string out = "wxLC_REPORT";
    if (containsToken(rcStyle, "WS_BORDER")) out += " | wxBORDER_SIMPLE";
    return out;
}

CodeBuilder& createButton(const Control& ctl, const std::string& parent, CodeBuilder& c)
{
    c.stream() << c.pad() << "{\n";
    c.push();
    c.stream() << c.pad() << "auto* btn = new wxButton(this, " << idExpr << ", " << cppStringLiteral(ctl.m_control.text) << ");\n";
    c.stream() << c.pad() << parent << "->Add(btn);\n";
    if (ctl.m_control.kind == RcControl::Type::DefPushButton) c.stream() << c.pad() << "btn->SetDefault();\n";
    c.pop();
    c.stream() << c.pad() << "}\n";
    return c;
}

CodeBuilder& createStaticText(const Control& ctl, const std::string& parent, CodeBuilder& c)
{
    c.stream() << c.pad() << parent << "->Add(new wxStaticText(this, " << idExpr << ", " << cppStringLiteral(ctl.m_control.text)
        << "), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);\n";
    return c;
}

CodeBuilder& createStaticBox(const Control& ctl, const std::string& parent, CodeBuilder& c)
{
    c.stream() << c.pad() << "{\n";
    c.push();
    c.stream() << c.pad() << "auto sizer = new wxStaticBoxSizer(wxVERTICAL, this, " << cppStringLiteral(ctl.m_control.text) <<  ");\n";
    auto children(ctl.m_children);
    std::sort(children.begin(), children.end(), [](const Control& a, const Control& b) {
        return a.m_control.rectDU.left < b.m_control.rectDU.left;
    });
    for (const auto& child : children) {
        if (child.m_type == Control::Type::PushButton)
            createControl(child, "sizer", c);
        else
            createControl(child, "sizer", c);
    }
    c.stream() << c.pad() << parent << "->Add(sizer, 0, wxEXPAND | wxALL, 5);\n";
    c.pop();
    c.stream() << c.pad() << "}\n";
    return c;
}

CodeBuilder& createEditText(const Control& ctl, const std::string& parent, CodeBuilder& c)
{
    c.stream() << c.pad() << "auto editText = new wxTextCtrl(this, " << idExpr << ", \"" << ctl.m_control.id << "\"" //wxEmptyString"
        << ", wxDefaultPosition, wxDefaultSize"
        << ", " << wxTextCtrlStyleFromRc(ctl.m_control.style) << ");\n";
    c.stream() << c.pad() << parent << "->Add(editText);\n";
    return c;
}

CodeBuilder& createListview(const Control& ctl, const std::string& parent, CodeBuilder& c)
{
    c.stream() << c.pad() << "auto listCtrl = new wxListCtrl(this, " << idExpr
        << ", wxDefaultPosition, wxDefaultSize"
        << ", " << wxListCtrlStyleFromRc(ctl.m_control.style) << ");\n";
    c.stream() << c.pad() << parent << "->Add(listCtrl, 1, wxEXPAND | wxALL, 5);\n";
    return c;
}

CodeBuilder& createRadioButton(const Control& ctl, const std::string& parent, CodeBuilder& c)
{
    c.stream() << c.pad() << "auto radioBtn = new wxRadioButton(this, " << idExpr << ", " << cppStringLiteral(ctl.m_control.text) << ");\n";
    c.stream() << c.pad() << parent << "->Add(radioBtn);\n";
    return c;
}

CodeBuilder& createComboBox(const Control& ctl, const std::string& parent, CodeBuilder& builder)
{
    builder.stream() << builder.pad() << parent << "->Add(new wxComboBox(this, " << idExpr << ", wxEmptyString));\n";
    return builder;
}

CodeBuilder& createLine(const Control& ctl, const std::string& parent, CodeBuilder& builder)
{
    builder.stream() << builder.pad() << "{\n";
    builder.push();
    builder.stream() << builder.pad() << "auto box = new wxBoxSizer(wxHORIZONTAL);\n";
    auto children(ctl.m_children);
    std::sort(children.begin(), children.end(), [] (const Control& a, const Control& b) {
        return a.m_control.rectDU.left < b.m_control.rectDU.left;
              });
    for (const auto& child : children) {
        createControl(child, "box", builder);
    }
    builder.stream() << builder.pad() << parent<< "->Add(box, 0, wxEXPAND | wxALL, 5); \n";
    builder.pop();
    builder.stream() << builder.pad()<< "}\n";
    return builder;
}

CodeBuilder& createControl(const Control& ctl, const std::string& parent, CodeBuilder& builder)
{
    switch (ctl.m_type) {
        case Control::Type::StaticText:
            return createStaticText(ctl, parent, builder);
        case Control::Type::GroupBox:
            return createStaticBox(ctl, parent, builder);
        case Control::Type::Line:
            return createLine(ctl, parent, builder);
        case Control::Type::EditText:
            return createEditText(ctl, parent, builder);
        case Control::Type::ComboBox:
            return createComboBox(ctl, parent, builder);
        case Control::Type::PushButton:
            return createButton(ctl, parent, builder);
        case Control::Type::ListView:
            return createListview(ctl, parent, builder);
        case Control::Type::RadioButton:
            return createRadioButton(ctl, parent, builder);
        case Control::Type::TabControl:
            builder.stream() << builder.pad() << parent << "->Add(new wxNotebook(this, " << idExpr << "));\n";
            break;
        case Control::Type::Icon:
            builder.stream() << builder.pad() << "// TODO: ICON " << cppStringLiteral(ctl.m_control.id) << "\n";
            break;
        case Control::Type::Control:
            builder.stream() << builder.pad() << "// TODO: CONTROL class " << cppStringLiteral(ctl.m_control.winClass) << "\n";
            builder.stream() << builder.pad() << parent << "->Add(new wxWindow(this, " << idExpr << "));\n";
            break;
        default:
            throw std::invalid_argument("Unknown control type");
    }
    return builder;
}

} // namespace

WxEmitResult WxEmitter::emit(const Dialog& dialog) const
{
    std::ostringstream height;
    std::ostringstream c;
    const std::string cls = "Rc" + sanitizeIdent(dialog.name);

    height << "#pragma once\n\n";
    height << "#include <wx/wx.h>\n";
    height << "#include <wx/listctrl.h>\n";
    height << "#include <wx/notebook.h>\n\n";

    height << "// Generated by rc2WxWidgets (one-time conversion)\n\n";

    c << "#include \"" << cls << ".h\"\n\n";
    c << "// Generated by rc2WxWidgets (one-time conversion)\n\n";

    WxEmitResult out;
    out.className = cls;

    if (dialog.isChild()) {
        height << "class " << cls << " : public wxPanel\n{\npublic:\n";
        height << "    explicit " << cls << "(wxWindow* parent);\n";
        height << "};\n\n";

        c << cls << "::" << cls << "(wxWindow* parent)\n";
        c << "    : wxPanel(parent, wxID_ANY)\n{\n";
    }
    else {
        height << "class " << cls << " : public wxDialog\n{\npublic:\n";
        height << "    explicit " << cls << "(wxWindow* parent);\n";
        height << "};\n\n";

        c << cls << "::" << cls << "(wxWindow* parent)\n";
        c << "    : wxDialog(parent, wxID_ANY, " << cppStringLiteral(dialog.caption.empty() ? dialog.name : dialog.caption)
            << ", wxDefaultPosition, wxDefaultSize,\n";
        c << "        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)\n{\n";
    }

    if (dialog.fontPointSize > 0 && !dialog.fontFace.empty()) {
        c << "    SetFont(wxFontInfo(" << dialog.fontPointSize << ").FaceName("
            << cppStringLiteral(dialog.fontFace) << "));\n";
    }

    // Size in dialog units; let wx convert to pixels.
    c << "    SetClientSize(ConvertDialogToPixels(wxSize(" << dialog.rectDU.width << ", " << dialog.rectDU.height << ")));\n\n";
    c << "    auto* mainSizer = new wxBoxSizer(wxVERTICAL);\n";

    CodeBuilder builder(c);
    for (const auto& ctl : dialog.controls) {
        if (ctl.m_control.kind == RcControl::Type::DefPushButton)
            createControl(ctl, "mainSizer", builder);
        else if (ctl.m_control.kind == RcControl::Type::PushButton)
            createControl(ctl, "mainSizer", builder);
        else
            createControl(ctl, "mainSizer", builder);
    }

    c << "    SetSizerAndFit(mainSizer);\n";
    c << "    Layout();\n";

    c << "}\n\n";

    out.header = height.str();
    out.source = c.str();
    return out;
}

} // namespace wxConvert