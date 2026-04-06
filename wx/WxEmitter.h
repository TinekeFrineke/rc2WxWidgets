#pragma once

#include <string>

#include "../rc/RcModel.h"

struct WxEmitResult
{
    std::string header;
    std::string source;
};

class WxEmitter
{
public:
    WxEmitResult emit(const RcFile& rc) const;
};

