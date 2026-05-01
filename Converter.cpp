#include "Converter.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "rc/RcParser.h"
#include "text/TextFile.h"
#include "wx/WxEmitter.h"

namespace wxConvert {

Converter::Converter(std::string input, std::string output)
    : m_input(std::move(input))
    , m_output(std::move(output))
{
}

void Converter::convert()
{
    namespace fs = std::filesystem;

    auto currentPath(std::filesystem::current_path());
    std::string textUtf8;
    try {
        textUtf8 = readFileAsUtf8(m_input);
    }
    catch (const std::exception& e) {
        std::cout << "Input read failed: " << e.what() << "\n";
        return;
    }
    if (textUtf8.empty()) return;

    RcParser parser;
    std::istringstream inStream(textUtf8);
    RcFile rc;
    try {
        rc = parser.parse(inStream);
    }
    catch (const std::exception& e) {
        std::cout << "Parsing failed: " << e.what() << "\n";
        return;
    }

    fs::path outDir(m_output);
    fs::create_directories(outDir);

    WxEmitter emitter;
    const auto result = emitter.emit(rc);

    for (const auto& out : result)
    {
        std::string outHeaderFile = out.className + ".h";
        {
            std::ofstream hpp(outDir / outHeaderFile, std::ios::binary);
            if (!hpp) throw std::runtime_error("Could not write " + outHeaderFile);
            hpp << out.header;
        }
        std::string outSourceFile = out.className + ".cpp";
        {
            std::ofstream cpp(outDir / outSourceFile, std::ios::binary);
            if (!cpp) throw std::runtime_error("Could not write " + outSourceFile);
            cpp << out.source;
        }
        std::cout << "Wrote: " << (outDir / outHeaderFile).string() << "\n";
        std::cout << "Wrote: " << (outDir / outSourceFile).string() << "\n";
    }

    std::cout << "Parsed dialogs: " << rc.dialogs.size() << "\n";


}

} // namespace wxConvert