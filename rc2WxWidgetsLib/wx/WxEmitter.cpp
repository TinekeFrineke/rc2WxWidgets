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

CodeBuilder& createButton(const Control& ctl, const std::string& parent, CodeBuilder& builder)
{
    builder.headerStream() << "    wxButton* " << ctl.m_control.memberName << ";\n";
    builder.cppStream() << builder.pad() << "{\n";
    builder.push();
    builder.cppStream() << builder.pad() << ctl.m_control.memberName << " = new wxButton(this, " << idExpr << ", " << cppStringLiteral(ctl.m_control.text) << ");\n";
    builder.cppStream() << builder.pad() << parent << "->Add(" << ctl.m_control.memberName << ");\n";
    if (ctl.m_control.kind == RcControl::Type::DefPushButton) builder.cppStream() << builder.pad() << ctl.m_control.memberName << "->SetDefault();\n";
    builder.pop();
    builder.cppStream() << builder.pad() << "}\n";
    return builder;
}

CodeBuilder& createStaticText(const Control& ctl, const std::string& parent, CodeBuilder& builder)
{
    builder.cppStream() << builder.pad() << parent << "->Add(new wxStaticText(this, " << idExpr << ", " << cppStringLiteral(ctl.m_control.text)
        << "), 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);\n";
    return builder;
}

CodeBuilder& createStaticBox(const Control& ctl, const std::string& parent, CodeBuilder& builder)
{
    builder.cppStream() << builder.pad() << "{\n";
    builder.push();
    builder.cppStream() << builder.pad() << "auto sizer = new wxStaticBoxSizer(wxVERTICAL, this, " << cppStringLiteral(ctl.m_control.text) <<  ");\n";
    auto children(ctl.m_children);
    std::sort(children.begin(), children.end(), [](const Control& a, const Control& b) {
        return a.m_control.rectDU.left < b.m_control.rectDU.left;
    });
    for (const auto& child : children) {
        if (child.m_type == Control::Type::PushButton)
            createControl(child, "sizer", builder);
        else
            createControl(child, "sizer", builder);
    }
    builder.cppStream() << builder.pad() << parent << "->Add(sizer, 0, wxEXPAND | wxALL, 5);\n";
    builder.pop();
    builder.cppStream() << builder.pad() << "}\n";
    return builder;
}

CodeBuilder& createEditText(const Control& ctl, const std::string& parent, CodeBuilder& builder)
{
    builder.headerStream() << "    wxTextCtrl* " << ctl.m_control.memberName << ";\n";
    builder.cppStream() << builder.pad() << ctl.m_control.memberName << " = new wxTextCtrl(this, " << idExpr << ", wxEmptyString"
        << ", wxDefaultPosition, wxDefaultSize"
        << ", " << wxTextCtrlStyleFromRc(ctl.m_control.style) << ");\n";
    builder.cppStream() << builder.pad() << ctl.m_control.memberName << "->SetMinSize(wxSize(150, -1));\n";
    builder.cppStream() << builder.pad() << parent << "->Add(" << ctl.m_control.memberName << ", 2, wxEXPAND);\n";
    return builder;
}

CodeBuilder& createListview(const Control& ctl, const std::string& parent, CodeBuilder& builder)
{
    builder.headerStream() << "    wxListCtrl* " << ctl.m_control.memberName << ";\n";
    builder.cppStream() << builder.pad() << ctl.m_control.memberName << " = new wxListCtrl(this, " << idExpr
        << ", wxDefaultPosition, wxDefaultSize"
        << ", " << wxListCtrlStyleFromRc(ctl.m_control.style) << ");\n";
    builder.cppStream() << builder.pad() << parent << "->Add(" << ctl.m_control.memberName << ", 1, wxEXPAND | wxALL, 5);\n";
    return builder;
}

CodeBuilder& createRadioButton(const Control& ctl, const std::string& parent, CodeBuilder& builder)
{
    builder.headerStream() << "    wxRadioButton* " << ctl.m_control.memberName << ";\n";
    builder.cppStream() << builder.pad() << ctl.m_control.memberName << " = new wxRadioButton(this, " << idExpr << ", " << cppStringLiteral(ctl.m_control.text) << ");\n";
    builder.cppStream() << builder.pad() << parent << "->Add(" << ctl.m_control.memberName << ");\n";
    return builder;
}

CodeBuilder& createNotepad(const Control& ctl, const std::string& parent, CodeBuilder& builder)
{
    builder.headerStream() << "    wxNotebook* " << ctl.m_control.memberName << ";\n";
    builder.cppStream() << builder.pad() << ctl.m_control.memberName << " = new wxNotebook(this, " << idExpr << ");\n";
    builder.cppStream() << builder.pad() << parent << "->Add(" << ctl.m_control.memberName << "); \n";
    return builder;
}

CodeBuilder& createComboBox(const Control& ctl, const std::string& parent, CodeBuilder& builder)
{
    builder.headerStream() << "    wxComboBox* " << ctl.m_control.memberName << ";\n";
    builder.cppStream() << builder.pad() << ctl.m_control.memberName << " = new wxComboBox(this, " << idExpr << ", wxEmptyString);\n";
    builder.cppStream() << builder.pad() << parent << "->Add(" << ctl.m_control.memberName << "); \n";
    return builder;
}

CodeBuilder& createLine(const Control& ctl, const std::string& parent, CodeBuilder& builder)
{
    builder.cppStream() << builder.pad() << "{\n";
    builder.push();
    builder.cppStream() << builder.pad() << "auto box = new wxBoxSizer(wxHORIZONTAL);\n";
    auto children(ctl.m_children);
    std::sort(children.begin(), children.end(), [] (const Control& a, const Control& b) {
        return a.m_control.rectDU.left < b.m_control.rectDU.left;
    });
    for (const auto& child : children) {
        createControl(child, "box", builder);
    }
    builder.cppStream() << builder.pad() << parent<< "->Add(box, 0, wxEXPAND | wxALL, 5); \n";
    builder.pop();
    builder.cppStream() << builder.pad()<< "}\n";
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
            return createNotepad(ctl, parent, builder);
        case Control::Type::Icon:
            builder.cppStream() << builder.pad() << "// TODO: ICON " << cppStringLiteral(ctl.m_control.id) << "\n";
            break;
        case Control::Type::Control:
            builder.cppStream() << builder.pad() << "// TODO: CONTROL class " << cppStringLiteral(ctl.m_control.winClass) << "\n";
            builder.cppStream() << builder.pad() << parent << "->Add(new wxWindow(this, " << idExpr << "));\n";
            break;
        default:
            throw std::invalid_argument("Unknown control type");
    }
    return builder;
}

} // namespace

WxEmitResult WxEmitter::emit(const Dialog& dialog) const
{
    std::ostringstream header;
    std::ostringstream source;
    const std::string cls = "Rc" + sanitizeIdent(dialog.name);

    header << "#pragma once\n\n";
    header << "#include <wx/wx.h>\n";
    header << "#include <wx/listctrl.h>\n";
    header << "#include <wx/notebook.h>\n\n";

    header << "// Generated by rc2WxWidgets (one-time conversion)\n\n";

    source << "#include \"" << cls << ".h\"\n\n";
    source << "// Generated by rc2WxWidgets (one-time conversion)\n\n";

    WxEmitResult out;
    out.className = cls;

    if (dialog.isChild()) {
        header << "class " << cls << " : public wxPanel\n{\npublic:\n";

        source << cls << "::" << cls << "(wxWindow* parent)\n";
        source << "    : wxPanel(parent, wxID_ANY)\n{\n";
    }
    else {
        header << "class " << cls << " : public wxDialog\n{\npublic:\n";

        source << cls << "::" << cls << "(wxWindow* parent)\n";
        source << "    : wxDialog(parent, wxID_ANY, " << cppStringLiteral(dialog.caption.empty() ? dialog.name : dialog.caption)
            << ", wxDefaultPosition, wxDefaultSize,\n";
        source << "        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)\n{\n";
    }

    if (dialog.fontPointSize > 0 && !dialog.fontFace.empty()) {
        source << "    SetFont(wxFontInfo(" << dialog.fontPointSize << ").FaceName("
            << cppStringLiteral(dialog.fontFace) << "));\n";
    }

    // Size in dialog units; let wx convert to pixels.
    source << "    SetClientSize(ConvertDialogToPixels(wxSize(" << dialog.rectDU.width << ", " << dialog.rectDU.height << ")));\n\n";
    source << "    auto* mainSizer = new wxBoxSizer(wxVERTICAL);\n";

    CodeBuilder builder(source, header);
    for (const auto& ctl : dialog.controls) {
        createControl(ctl, "mainSizer", builder);
    }

    source << "    SetSizerAndFit(mainSizer);\n";
    source << "    Layout();\n";

    source << "}\n\n";

    header << "    explicit " << cls << "(wxWindow* parent);\n";
    header << "};\n\n";

    out.header = header.str();
    out.source = source.str();
    return out;
}

} // namespace wxConvert