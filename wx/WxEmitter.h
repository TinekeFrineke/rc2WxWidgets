#pragma once

#include <string>
#include <vector>

#include "../rc/RcModel.h"

struct WxEmitResult
{
    std::string className;
    std::string header;
    std::string source;
};

class WxEmitter
{
public:
    std::vector<WxEmitResult> emit(const RcFile& rc) const;
};

