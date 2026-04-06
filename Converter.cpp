#include "Converter.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "rc/RcParser.h"
#include "text/TextFile.h"
#include "wx/WxEmitter.h"

Converter::Converter(std::string input, std::string output)
    : m_input(std::move(input))
    , m_output(std::move(output))
{
}

void Converter::convert()
{
    namespace fs = std::filesystem;

    std::string textUtf8;
    try {
        textUtf8 = readFileAsUtf8(m_input);
    } catch (const std::exception& e) {
        std::cout << "Input read failed: " << e.what() << "\n";
        return;
    }
    if (textUtf8.empty()) return;

    RcParser parser;
    std::istringstream inStream(textUtf8);
    RcFile rc = parser.parse(inStream);

    fs::path outDir(m_output);
    fs::create_directories(outDir);

    WxEmitter emitter;
    const auto out = emitter.emit(rc);

    {
        std::ofstream hpp(outDir / "DialogsFromRc.h", std::ios::binary);
        if (!hpp) throw std::runtime_error("Could not write DialogsFromRc.h");
        hpp << out.header;
    }
    {
        std::ofstream cpp(outDir / "DialogsFromRc.cpp", std::ios::binary);
        if (!cpp) throw std::runtime_error("Could not write DialogsFromRc.cpp");
        cpp << out.source;
    }

    std::cout << "Parsed dialogs: " << rc.dialogs.size() << "\n";
    std::cout << "Wrote: " << (outDir / "DialogsFromRc.h").string() << "\n";
    std::cout << "Wrote: " << (outDir / "DialogsFromRc.cpp").string() << "\n";


}
