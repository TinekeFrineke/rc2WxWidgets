#include "Converter.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include "rc/RcParser.h"
#include "text/TextFile.h"
#include "wx/DialogInterpreter.h"
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

    std::cout << "Parsed dialogs: " << rc.dialogs.size() << "\n";

    fs::path outDir(m_output);
    fs::create_directories(outDir);

    for (const auto& rcDialog : rc.dialogs) {
        auto wxDialog = dialogInterpreter::interpret(rcDialog);

        WxEmitter emitter;
        const auto result = emitter.emit(wxDialog);

        std::string outHeaderFile = result.className + ".h";
        {
            std::ofstream hpp(outDir / outHeaderFile, std::ios::binary);
            if (!hpp) throw std::runtime_error("Could not write " + outHeaderFile);
            hpp << result.header;
        }
        std::string outSourceFile = result.className + ".cpp";
        {
            std::ofstream cpp(outDir / outSourceFile, std::ios::binary);
            if (!cpp) throw std::runtime_error("Could not write " + outSourceFile);
            cpp << result.source;
        }
        std::cout << "Wrote: " << (outDir / outHeaderFile).string() << "\n";
        std::cout << "Wrote: " << (outDir / outSourceFile).string() << "\n";
    }
    std::cout << "Wrote " << rc.dialogs.size() << " dialogs\n";

}

} // namespace wxConvert