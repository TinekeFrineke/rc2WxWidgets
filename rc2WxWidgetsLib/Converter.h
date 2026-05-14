#pragma once


#include <string>

namespace wxConvert {

class Converter
{
public:
    Converter(std::string input, std::string projectName, std::string output);

    void convert();

private:
    std::string m_input;
    std::string m_output;
    std::string m_projectName;
};

} // namespace wxConvert