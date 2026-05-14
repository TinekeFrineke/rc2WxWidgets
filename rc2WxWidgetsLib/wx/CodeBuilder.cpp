
#include "CodeBuilder.h"

#include <sstream>

namespace wxConvert {

namespace {
int tabSize = 4;
} // namespace

CodeBuilder::CodeBuilder(std::ostream& cppStream, std::ostream& headerStream)
    : m_cppStream(cppStream)
    , m_headerStream(headerStream)
{
}

std::ostream& CodeBuilder::cppStream()
{
    return m_cppStream;
}

std::ostream& CodeBuilder::headerStream()
{
    return m_headerStream;
}

std::string CodeBuilder::pad() const
{
    std::string output;
    for (int i = 0; i < (m_indent + 1) * tabSize; ++i) {
        output += ' ';
    }
    return output;
}


void CodeBuilder::push()
{
    ++m_indent;
}

void CodeBuilder::pop()
{
    --m_indent;
}

} // namespace wxConvert