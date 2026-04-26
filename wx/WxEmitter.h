#pragma once

#include <map>
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
    std::map<std::string, WxEmitResult> emit(const RcFile& rc) const;
};

