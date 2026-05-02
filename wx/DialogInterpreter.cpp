
#include "DialogInterpreter.h"

#include <algorithm>

namespace wxConvert {

Control::Control(const RcControl & control) : m_control(control) {}

Control::~Control() = default;


namespace dialogInterpreter {

namespace {
std::vector<std::unique_ptr<Control>> getChildren(const RcRectDU& clientRectangle, std::vector<RcControl>& candidates)
{
    std::vector<std::unique_ptr<Control>> children;
    std::vector<RcControl>::iterator iter = candidates.begin();
    while (iter != candidates.end()) {
        if (isInside(clientRectangle, iter->rectDU)) {
            children.push_back(std::make_unique<Control>(*iter));
            if (iter->kind == RcControlKind::GroupBox)
                children.back()->m_children = getChildren(children.back()->m_control.rectDU, candidates);
            iter = candidates.erase(iter);
        }
        else {
            ++iter;
        }
    }

    return std::move(children);
}

void nestGroupBoxes(std::vector<std::unique_ptr<Control>>& groupBoxes)
{
    // Determine overlapping
    std::sort(groupBoxes.begin(), groupBoxes.end(), [] (const std::unique_ptr<Control>& lhs, const std::unique_ptr<Control>& rhs)
              {
                  return lhs->m_control.rectDU.surface() < rhs->m_control.rectDU.surface();
              });

    std::vector<std::unique_ptr<Control>> newGroupBoxes;

    while (!groupBoxes.empty()) {
        newGroupBoxes.push_back(std::move(groupBoxes.front()));
        groupBoxes.erase(groupBoxes.begin());
        auto& box = *newGroupBoxes.back();
        auto candidates = groupBoxes.begin();
        while (candidates != groupBoxes.end()) {

        }
    }
    auto iter = groupBoxes.begin();
    while (iter != groupBoxes.end()) {
        newGroupBoxes.push_back(*iter);
        auto& box = *iter;
        auto canditer = iter + 1;
        while (canditer != groupBoxes.end()) {
            if (iter->kind == RcControlKind::GroupBox) {
            groupBoxes.push_back(std::make_unique<Control>(*iter));
            iter = dialog.controls.erase(iter);
        }
        else {
            ++iter;
        }
    }
}

} // namespace

std::vector<std::unique_ptr<Control>> interpret(const RcDialog& rcDialog)
{
    auto dialog(rcDialog);

    std::vector<std::unique_ptr<Control>> groupBoxes;
    std::vector<RcControl>::iterator iter = dialog.controls.begin();
    while (iter != dialog.controls.end()) {
        if (iter->kind == RcControlKind::GroupBox) {
            groupBoxes.push_back(std::make_unique<Control>(*iter));
            iter = dialog.controls.erase(iter);
        }
        else {
            ++iter;
        }
    }

    for (const auto& groupBox : groupBoxes) {
        groupBox->m_children = std::move(getChildren(groupBox->m_control.rectDU, dialog.controls));
    }

    for (auto& control : dialog.controls)
        groupBoxes.push_back(std::make_unique<Control>(control));
    return groupBoxes;
}

} // namespace dialogInterpreter
} // namespace wxConvert