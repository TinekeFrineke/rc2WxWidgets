#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct RcRectDU
{
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
};

enum class RcControlKind
{
    Unknown,
    LText,
    CText,
    RText,
    GroupBox,
    EditText,
    ComboBox,
    PushButton,
    DefPushButton,
    Icon,
    Control, // generic CONTROL line with explicit window class
};

struct RcControl
{
    RcControlKind kind = RcControlKind::Unknown;
    std::string text;      // may be empty
    std::string id;        // e.g. IDC_FOO or IDOK
    std::string winClass;  // for CONTROL lines, e.g. "SysListView32"
    std::string style;     // raw style/exstyle token tail
    RcRectDU rectDU{};
};

struct RcDialog
{
    std::string name;   // e.g. IDD_ABOUTBOX
    RcRectDU rectDU{};

    std::string style;
    std::string exStyle;
    std::string caption;
    int fontPointSize = 0;
    std::string fontFace;

    std::vector<RcControl> controls;

    bool isChild() const;
};

struct RcFile
{
    std::vector<RcDialog> dialogs;
};

