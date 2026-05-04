#pragma once

#include <string>
#include <vector>

#include "RcModel.h"

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
    WxEmitResult emit(const RcDialog& rc) const;
};

} // namespace wxConvert