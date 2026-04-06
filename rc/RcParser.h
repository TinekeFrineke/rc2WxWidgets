#pragma once

#include <istream>

#include "RcModel.h"

class RcParser
{
public:
    RcFile parse(std::istream& in) const;
};

