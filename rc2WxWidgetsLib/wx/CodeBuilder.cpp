
#include "CodeBuilder.h"

#include <sstream>

namespace wxConvert {

namespace {
int tabSize = 4;
} // namespace

CodeBuilder::CodeBuilder(std::ostream& output)
    : m_output(output)
{
}

std::ostream& CodeBuilder::stream()
{
    return m_output;
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