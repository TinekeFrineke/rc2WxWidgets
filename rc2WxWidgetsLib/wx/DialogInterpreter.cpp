
#include "DialogInterpreter.h"

#include <algorithm>
#include <stdexcept>

namespace wxConvert {

namespace {
Control::Type toType(const RcControl& rcControl)
{
    switch (rcControl.kind) {
        case RcControl::Type::GroupBox:
            return Control::Type::GroupBox;
        case RcControl::Type::LText:
        case RcControl::Type::CText:
        case RcControl::Type::RText:
            return Control::Type::StaticText;
        case RcControl::Type::EditText:
            return Control::Type::EditText;
        case RcControl::Type::ComboBox:
            return Control::Type::ComboBox;
        case RcControl::Type::PushButton:
        case RcControl::Type::DefPushButton:
            return Control::Type::PushButton;
        case RcControl::Type::Icon:
            return Control::Type::Icon;
        case RcControl::Type::Control:
            if (rcControl.winClass == "SysListView32")
                return Control::Type::ListView;
            else if (rcControl.winClass == "SysTabControl32")
                return Control::Type::TabControl;
            else if (rcControl.winClass == "Button" && rcControl.style.find("BS_AUTORADIOBUTTON") != std::string::npos)
                return Control::Type::RadioButton;
            else
                return Control::Type::Control;
        case RcControl::Type::Other:
            return Control::Type::Line;
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
    , m_type(toType(control))
{}

Control::~Control() = default;

namespace dialogInterpreter {

namespace {

bool addNestedControl(Control& parent, const Control& child)
{
    if (!parent.m_children.empty())
        for (auto& nested : parent.m_children)
            if (addNestedControl(nested, child))
                return true;

    if (isInside(parent.m_control.rectDU, child.m_control.rectDU)) {
        parent.m_children.push_back(child);
        return true;
    }
    return false;
}

bool addNestedControl(std::vector<Control>::iterator start,
                      std::vector<Control>::iterator end,
                      const Control& child)
{
    for (auto it = start; it != end; ++it) {
        if (addNestedControl(*it, child))
            return true;
    }
    return false;
}

void nestGroupBoxes(std::vector<Control>& groupBoxes)
{
    const size_t n = groupBoxes.size();
    if (n <= 1)
        return;

    // Move input into indexed storage to avoid erases while preserving order.
    std::sort(groupBoxes.begin(), groupBoxes.end(), [] (const Control& lhs, const Control& rhs) {
        return lhs.m_control.rectDU.surface() < rhs.m_control.rectDU.surface();
    });

    auto currentBox = groupBoxes.begin();
    while (currentBox != groupBoxes.end()) {
        if (addNestedControl(std::next(currentBox), groupBoxes.end(), *currentBox)) {
            currentBox = groupBoxes.erase(currentBox);
        }
        else {
            ++currentBox;
        }
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
            groupBoxes.push_back(std::move(Control(control)));
        }
        else {
            commonControls.push_back(std::move(Control(control)));
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

std::vector<Control> aggregateLines(std::vector<Control> controls)
{
    // aggregate groupboxes first
    for (auto& control : controls) {
        if (control.m_type == Control::Type::GroupBox)
            control.m_children = aggregateLines(std::move(control.m_children));
    }

    // sort from top to bottom
    std::sort(controls.begin(), controls.end(), [](const Control& a, const Control& b) {
        return a.m_control.rectDU.top < b.m_control.rectDU.top;
    });

    // Now the controls are sorted top to bottom. Filter out "other" lines.
    std::vector<Control> result;
    while (!controls.empty()) {
        if (controls.size() == 1) {
            result.push_back(std::move(controls.front()));
            return result;
        }

        std::vector<Control> lineCandidates;
        // start with the highest control
        lineCandidates.push_back(std::move(controls.front()));
        auto& candidate = lineCandidates.front();
        auto candidateInterval = verticalInterval(candidate.m_control.rectDU);
        auto nextCandidate = controls.erase(controls.begin());
        RcRectDU lineRect;
        if (nextCandidate->m_control.text == "Delete")
            lineRect  = candidate.m_control.rectDU;
        else
            lineRect = candidate.m_control.rectDU;
        while (nextCandidate != controls.end() && overlapRatio(verticalInterval(nextCandidate->m_control.rectDU), candidateInterval) > 0.5) {
            const auto nextInterval = verticalInterval(nextCandidate->m_control.rectDU);
            lineRect.add(nextCandidate->m_control.rectDU);
            lineCandidates.push_back(std::move(*nextCandidate));
            if (nextCandidate->m_control.text == "Delete")
                lineRect = lineRect;
            nextCandidate = controls.erase(nextCandidate);
        }

        if (lineCandidates.size() >= 2) {
            // aggregate line candidates into a single line control
            Control lineControl{ RcControl{ RcControl::Type::Other, lineRect } };
            lineControl.m_children = std::move(lineCandidates);
            result.push_back(std::move(lineControl));
        }
        else {
            result.push_back(std::move(lineCandidates.front()));
        }
    }

    return result;
}
} // namespace

Dialog interpret(const RcDialog& rcDialog)
{
    auto dialog(rcDialog);
    auto controls(rcDialog.controls);

    auto aggregatedControls = aggregateGroupBoxes(std::move(controls));
    aggregatedControls = aggregateLines(std::move(aggregatedControls));
    return { .name = rcDialog.name, .rectDU = rcDialog.rectDU, .style = rcDialog.style,
        .exStyle = rcDialog.exStyle, .caption = rcDialog.caption, .fontPointSize = rcDialog.fontPointSize,
        .fontFace = rcDialog.fontFace, .controls = std::move(aggregatedControls) };
}

} // namespace dialogInterpreter
} // namespace wxConvert