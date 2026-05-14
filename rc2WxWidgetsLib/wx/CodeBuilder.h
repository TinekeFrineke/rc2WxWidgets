
#pragma once

#include <iosfwd>
#include <string>

namespace wxConvert {

class CodeBuilder
{
public:
    CodeBuilder(std::ostream& cppStream, std::ostream& headerStream);
    std::ostream& cppStream();
    std::ostream& headerStream();
    void push();
    void pop();
    std::string pad() const;

private:
    std::ostream& m_cppStream;
    std::ostream& m_headerStream;
    int m_indent{};
};

} // namespace wxConvert