
#pragma once

#include <memory>
#include <vector>

#include "RcModel.h"

namespace wxConvert {

struct Control
{
    Control(const RcControl& control);
    ~Control();

    RcControl m_control;
    std::vector<std::unique_ptr<Control>> m_children;
};

namespace dialogInterpreter {

std::vector<std::unique_ptr<Control>> interpret(const RcDialog& rcDialog);

} // namespace dialogInterpreter {
} // namespace wxConvert