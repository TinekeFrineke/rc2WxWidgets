#pragma once

#include <string>
#include <vector>

#include "RcModel.h"

#include "Control.h"

namespace wxConvert {

struct WxEmitResult
{
    std::string className;
    std::string header;
    std::string source;
};

class WxEmitter
{
public:
    WxEmitResult emit(const Dialog& rc) const;

private:
    std::string indent() const;

    //int m_indent{};
};

} // namespace wxConvert