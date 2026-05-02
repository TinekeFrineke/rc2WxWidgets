
#pragma once

#include <memory>
#include <vector>

#include "RcModel.h"

namespace wxConvert {

struct Control
{
    Control(const RcControl& control);
    ~Control();

    Control(Control&&) noexcept = default;
    Control& operator=(Control&&) noexcept = default;
#define TRY_CHEAP
#ifdef TRY_CHEAP
    Control(const Control&) = delete;
    Control& operator=(const Control&) = delete;
#else
    Control(const Control&) = default;
    Control& operator=(const Control&) = default;
#endif

    RcControl m_control;
    std::vector<Control> m_children;
};

namespace dialogInterpreter {

std::vector<Control> interpret(const RcDialog& rcDialog);

} // namespace dialogInterpreter {
} // namespace wxConvert