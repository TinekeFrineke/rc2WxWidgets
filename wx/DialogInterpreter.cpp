
#include "DialogInterpreter.h"

#include <algorithm>

namespace wxConvert {

Control::Control(const RcControl & control) : m_control(control) {}

Control::~Control() = default;

namespace dialogInterpreter {

namespace {
std::vector<Control> getChildren(const RcRectDU& clientRectangle, std::vector<RcControl>& candidates)
{
    std::vector<Control> children;
    std::vector<RcControl>::iterator iter = candidates.begin();
    while (iter != candidates.end()) {
        if (isInside(clientRectangle, iter->rectDU)) {
            children.push_back(Control(*iter));
            if (iter->kind == RcControl::Type::GroupBox)
                children.back().m_children = getChildren(children.back().m_control.rectDU, candidates);
            iter = candidates.erase(iter);
        }
        else {
            ++iter;
        }
    }

    return std::move(children);
}

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

} // namespace

std::vector<Control> interpret(const RcDialog& rcDialog)
{
    auto dialog(rcDialog);

    std::vector<Control> groupBoxes;
    std::vector<Control> commonControls;

    for (const auto& control : dialog.controls) {
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

} // namespace dialogInterpreter
} // namespace wxConvert