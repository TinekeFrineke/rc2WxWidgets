#pragma once


#include <string>

class Converter
{
public:
    Converter(std::string input, std::string output);

    void convert();

private:
    std::string m_input;
    std::string m_output;
};

