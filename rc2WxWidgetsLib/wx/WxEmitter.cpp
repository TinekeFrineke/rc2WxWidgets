#include "WxEmitter.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace wxConvert {

namespace {

const std::string idExpr = "wxID_ANY"; // safer default; user can map later
std::ostream& createControl(const Control& ctl, const std::string& parent, std::ostringstream& c);

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

std::ostringstream& createButton(const Control& ctl, const std::string& parent, std::ostringstream& c)
{
    c << "    {\n";
    c << "        auto* btn = new wxButton(this, " << idExpr << ", " << cppStringLiteral(ctl.m_control.text) << ");\n";
    c << "        " << parent << "->Add(btn);\n";
    if (ctl.m_control.kind == RcControl::Type::DefPushButton) c << "        btn->SetDefault();\n";
    c << "    }\n";
    return c;
}

std::ostringstream& createStaticText(const Control& ctl, const std::string& parent, std::ostringstream& c)
{
    c << "    " << parent << "->Add(new wxStaticText(this, " << idExpr << ", " << cppStringLiteral(ctl.m_control.text) << "));\n";
    return c;
}

std::ostringstream& createStaticBox(const Control& ctl, const std::string& parent, std::ostringstream& c)
{
    c << "    {\n";
    c << "    auto sizer = new wxStaticBoxSizer(wxVERTICAL, this, " << cppStringLiteral(ctl.m_control.text) <<  ");\n";
    c << "    " << parent << "->Add(sizer); \n";
    for (const auto& child : ctl.m_children) {
        createControl(child, "sizer", c);
    }
    c << "    mainSizer->Add(sizer, 0, wxEXPAND | wxALL, 5);\n";
    c << "    }\n";
    return c;
}

std::ostream& createEditText(const Control& ctl, const std::string& parent, std::ostringstream& c)
{
    c << "    new wxTextCtrl(this, " << idExpr << ", wxEmptyString"
        << ", wxDefaultPosition, wxDefaultSize"
        << ", " << wxTextCtrlStyleFromRc(ctl.m_control.style) << ");\n";
    return c;
}

std::ostream& createLine(const Control& ctl, const std::string& parent, std::ostringstream& c)
{
    c << "    {\n";
    c << "    auto box = new wxBoxSizer(wxHORIZONTAL);\n";
    for (const auto& child : ctl.m_children) {
        createControl(child, "box", c);
    }
    c << "    mainSizer->Add(box, 0, wxEXPAND | wxALL, 5);\n";
    c << "    }\n";
    return c;
}

std::ostream& createControl(const Control& ctl, const std::string& parent, std::ostringstream& c)
{
    switch (ctl.m_type) {
        case Control::Type::StaticText:
            return createStaticText(ctl, parent, c);
        case Control::Type::GroupBox:
            return createStaticBox(ctl, parent, c);
        case Control::Type::Line:
            return createLine(ctl, parent, c);
        case Control::Type::EditText:
            return createEditText(ctl, parent, c);
            break;
        case Control::Type::ComboBox:
            c << "    " << parent << "->Add(new wxComboBox(this, " << idExpr << ", wxEmptyString));\n";
            break;
        case Control::Type::PushButton:
            return createButton(ctl, parent, c);
        case Control::Type::Control:
            if (ctl.m_control.winClass == "SysListView32") {
                c << "    " << parent << "->Add(new wxListCtrl(this, " << idExpr
                    << ", wxDefaultPosition, wxDefaultSize"
                    << ", " << wxListCtrlStyleFromRc(ctl.m_control.style) << "));\n";
            }
            else if (ctl.m_control.winClass == "SysTabControl32") {
                c << "    " << parent << "->Add(new wxNotebook(this, " << idExpr << "));\n";
            }
            else if (ctl.m_control.winClass == "Button" && containsToken(ctl.m_control.style, "BS_AUTORADIOBUTTON")) {
                c << "    " << parent << "->Add(new wxRadioButton(this, " << idExpr << ", " << cppStringLiteral(ctl.m_control.text) << "));\n";
            }
            else {
                c << "    // TODO: CONTROL class " << cppStringLiteral(ctl.m_control.winClass) << "\n";
                c << "    " << parent << "->Add(new wxWindow(this, " << idExpr << "));\n";
            }
            break;
        case Control::Type::Icon:
            c << "    // TODO: ICON " << cppStringLiteral(ctl.m_control.id) << "\n";
            break;
        default:
            throw std::invalid_argument("Unknown control type");
    }
    return c;
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

    for (const auto& ctl : dialog.controls) {
        createControl(ctl, "mainSizer", c);
    }

    c << "    SetSizerAndFit(mainSizer);\n";
    c << "    Layout();\n";

    c << "}\n\n";

    out.header = height.str();
    out.source = c.str();
    return out;
}

} // namespace wxConvert