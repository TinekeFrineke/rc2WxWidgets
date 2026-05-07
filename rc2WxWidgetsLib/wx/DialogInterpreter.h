
#pragma once

#include <memory>
#include <vector>

#include "RcModel.h"

namespace wxConvert {

struct Control
{
    enum class Type
    {
        Unknown,
        StaticText,
        Editable,
        Line,
        GroupBox,
        Control,
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

namespace dialogInterpreter {

std::vector<Control> interpret(const RcDialog& rcDialog);

} // namespace dialogInterpreter {
} // namespace wxConvert