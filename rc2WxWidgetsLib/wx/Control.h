
#pragma once

#include <optional>
#include <string>
#include <vector>

#include "RcModel.h"

namespace wxConvert {

struct Control
{
    enum class Type
    {
        Unknown,
        StaticText,
        EditText,
        PushButton,
        ComboBox,
        Line,
        GroupBox,
        Control,
        Icon,
        ListView,
        TabControl,
        RadioButton
    };

    Control(const RcControl& control);
    ~Control();

    Control(Control&&) noexcept = default;
    Control& operator=(Control&&) noexcept = default;
    Control(const Control&) = default;
    Control& operator=(const Control&) = default;

    RcControl m_control;
    Type m_type;
    std::vector<Control> m_children;
};

struct Dialog
{
    std::string name;
    RcRectDU rectDU{};
    std::string style;
    std::string exStyle;
    std::string caption;
    int fontPointSize = 0;
    std::string fontFace;

    bool isChild() const { return style.find("WS_CHILD") != std::string::npos; }

    std::vector<Control> controls;
};

} // namespace wxConvert