
#include "DialogInterpreter.h"

#include <algorithm>
#include <stdexcept>

namespace wxConvert {

namespace {
Control::Type toType(const RcControl::Type& rcType)
{
    switch (rcType) {
    case RcControl::Type::GroupBox:
        return Control::Type::GroupBox;
    case RcControl::Type::LText:
    case RcControl::Type::CText:
    case RcControl::Type::RText:
        return Control::Type::StaticText;
    case RcControl::Type::EditText:
    case RcControl::Type::ComboBox:
        return Control::Type::Editable;
    case RcControl::Type::PushButton:
    case RcControl::Type::DefPushButton:
    case RcControl::Type::Icon:
    case RcControl::Type::Control:
        return Control::Type::Control;
    default:
        throw std::invalid_argument("Unknown RcControl::Type");
    }
}

bool heightOverlaps(const RcRectDU& lhs, const RcRectDU& rhs)
{
    int maxtop(std::max(lhs.top, rhs.top));
    int minbottom(std::min(lhs.bottom(), rhs.bottom()));
    return maxtop < minbottom;
}

} // namespace

Control::Control(const RcControl & control)
    : m_control(control)
    , m_type(toType(control.kind))
{}

Control::~Control() = default;

namespace dialogInterpreter {

namespace {

void nestGroupBoxes(std::vector<Control>& groupBoxes)
{
    const size_t n = groupBoxes.size();
    if (n <= 1)
        return;

    // Move input into indexed storage to avoid erases while preserving order.
    std::vector<Control> nodes;
    nodes.reserve(n);
    for (auto& g : groupBoxes)
        nodes.push_back(std::move(g));
    groupBoxes.clear();

    // Order indices by ascending area (smallest first).
    std::vector<size_t> order(n);
    for (size_t i = 0; i < n; ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [&nodes](size_t a, size_t b) {
        return nodes[a].m_control.rectDU.surface() < nodes[b].m_control.rectDU.surface();
    });

    // For each node, find the tightest (first) containing parent among larger nodes.
    std::vector<int> parentIndex(n, -1);
    for (size_t k = 0; k < n; ++k) {
        size_t i = order[k];
        for (size_t kk = k + 1; kk < n; ++kk) {
            size_t j = order[kk];
            if (isInside(nodes[j].m_control.rectDU, nodes[i].m_control.rectDU)) {
                parentIndex[i] = static_cast<int>(j);
                break;
            }
        }
    }

    // Collect children indices per parent.
    std::vector<std::vector<size_t>> childrenIdx(n);
    for (size_t i = 0; i < n; ++i) {
        if (parentIndex[i] != -1)
            childrenIdx[parentIndex[i]].push_back(i);
    }

    // Attach children bottom-up so each node has its subtree before being moved into its parent.
    for (size_t k = 0; k < n; ++k) {
        size_t idx = order[k];
        for (size_t childIdx : childrenIdx[idx]) {
            nodes[idx].m_children.push_back(std::move(nodes[childIdx]));
        }
    }

    // Collect top-level nodes (those without a parent). Keep large-first order.
    for (auto it = order.rbegin(); it != order.rend(); ++it) {
        size_t i = *it;
        if (parentIndex[i] == -1)
            groupBoxes.push_back(std::move(nodes[i]));
    }
}

bool addControl(Control& groupbox, Control& control)
{
    if (!groupbox.m_children.empty())
        for (auto& child : groupbox.m_children)
            if (addControl(child, control))
                return true;

    if (isInside(groupbox.m_control.rectDU, control.m_control.rectDU)) {
        groupbox.m_children.push_back(std::move(control));
        return true;
    }

    return false;
}

std::vector<Control> aggregateGroupBoxes(std::vector<RcControl>&& controls)
{
    std::vector<Control> groupBoxes;
    std::vector<Control> commonControls;

    for (const auto& control : controls) {
        if (control.kind == RcControl::Type::GroupBox) {
            groupBoxes.push_back(Control(control));
        }
        else {
            commonControls.push_back(Control(control));
        }
    }

    nestGroupBoxes(groupBoxes);

    auto controlIter = commonControls.begin();
    while (controlIter != commonControls.end()) {
        bool success = false;
        for (auto& groupBox : groupBoxes) {
            if (addControl(groupBox, *controlIter)) {
                success = true;
                break;
            }
        }
        if (success)
            controlIter = commonControls.erase(controlIter);
        else
            ++controlIter;
    }

    for (auto& groupBox : groupBoxes)
        commonControls.push_back(std::move(groupBox));

    return commonControls;
}

std::vector<Control> aggregateLines(std::vector<Control>&& controls)
{
    std::sort(controls.begin(), controls.end(), [](const Control& a, const Control& b) {
        return a.m_control.rectDU.top < b.m_control.rectDU.top;
              });

    // Now the controls are sorted top to bottom. Filter out "other" lines.
    std::vector<Control> lineCandidates;
    std::vector<Control> otherControls;
    for (auto& control : controls) {
        if (control.m_type == Control::Type::StaticText || control.m_type == Control::Type::Editable)
            lineCandidates.push_back(std::move(control));
        else
            otherControls.push_back(std::move(control));
    }

    // We can identify lines by grouping controls with overlapping top coordinates.

    //enum class State { WaitingStatic, WaitingText } state = State::WaitingStatic;
    //auto iterator = controls.begin();
    //while (iterator != controls.end()) {
    //    if (iterator->m_type == Control::Type::GroupBox) {
    //        aggregateLines(std::move(iterator->m_children));
    //        // Skip group boxes, they are handled separately.
    //        ++iterator;
    //        continue;
    //    }
    //    if (iterator->m_type == Control::Type::StaticText) {
    //        state = State::WaitingText;
    //        // Skip non-static text controls, lines start with static.
    //        ++iterator;
    //        continue;
    //    }

    //    // See whether we have a static followed by a line:
    //    if (iterator->m_type == Control::Type::Editable
    //        && heightOverlaps(iterator->m_control.rectDU, nextIter->m_control.rectDU)) {
    //        // We have a line, aggregate it:
    //        Control lineControl{ RcControl() };
    //        lineControl.m_type = Control::Type::Line;
    //        lineControl.m_children.push_back(std::move(*iterator));
    //        lineControl.m_children.push_back(std::move(*nextIter));
    //        iterator = controls.erase(iterator);
    //        nextIter = controls.erase(nextIter);
    //        controls.insert(iterator, std::move(lineControl));
    //    }
    //    else {
    //        // No line, move on.
    //        ++iterator;
    //    }
    //}

    return controls;
}
} // namespace

std::vector<Control> interpret(const RcDialog& rcDialog)
{
//    auto dialog(rcDialog);
    auto controls(rcDialog.controls);

    auto aggregatedControls = aggregateGroupBoxes(std::move(controls));
    aggregatedControls = aggregateLines(std::move(aggregatedControls));
    return aggregatedControls;
}

} // namespace dialogInterpreter
} // namespace wxConvert