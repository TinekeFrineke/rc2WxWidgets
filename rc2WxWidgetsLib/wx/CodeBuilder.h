
#pragma once

#include <iosfwd>
#include <string>

namespace wxConvert {

class CodeBuilder
{
public:
    CodeBuilder(std::ostream& output);
    std::ostream& stream();
    void push();
    void pop();
    std::string pad() const;

private:
    std::ostream& m_output;
    int m_indent{};
};

} // namespace wxConvert