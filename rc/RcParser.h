#pragma once

#include <istream>
#include <memory>

#include "RcModel.h"

struct RcDialog;

class RcParser
{
public:
    RcFile parse(std::istream& in) const;

    std::unique_ptr<RcDialog> parseDialog(std::istream& in) const;
};

