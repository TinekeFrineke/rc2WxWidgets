#include "RcModel.h"

namespace wxConvert {

namespace {
bool containsToken(const std::string& haystack, const char* needle)
{
    return haystack.find(needle) != std::string::npos;
}
}

bool RcDialog::isChild() const
{
    return containsToken(style, "WS_CHILD");
}

} // namespace wxConvert