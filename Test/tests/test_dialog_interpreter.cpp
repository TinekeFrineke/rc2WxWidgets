

#include <gtest/gtest.h>

#include "RcModel.h"
#include "wx/DialogInterpreter.h"

using namespace wxConvert;

TEST(DialogInterpreter, NestGroupBoxes_Simple)
{
    RcDialog dlg;
    dlg.name = "test";

    RcControl parent;
    parent.kind = RcControl::Type::GroupBox;
    parent.id = "P";
    parent.rectDU = RcRectDU{ 0,0,200,200 };
    RcControl child1;
    child1.kind = RcControl::Type::GroupBox;
    child1.id = "C1";
    child1.rectDU = RcRectDU{ 10,10,50,50 };
    RcControl child2;
    child2.kind = RcControl::Type::GroupBox;
    child2.id = "C2";
    child2.rectDU = RcRectDU{ 80,10,50,50 };

    // mixed order input
    dlg.controls = { child1, parent, child2 };

    auto result = dialogInterpreter::interpret(dlg);

    ASSERT_EQ(result.size(), 1u);
    const auto& top = result[0];
    EXPECT_EQ(top.m_control.id, "P");
    EXPECT_EQ(top.m_children.size(), 2u);

    std::set<std::string> ids;
    for (const auto& c : top.m_children) ids.insert(c.m_control.id);
    EXPECT_TRUE(ids.count("C1"));
    EXPECT_TRUE(ids.count("C2"));
}