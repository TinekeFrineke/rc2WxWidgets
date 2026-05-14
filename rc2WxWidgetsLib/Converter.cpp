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

namespace {

std::filesystem::path createSourceDir(const std::string& output, const std::string& projectName) {
    std::filesystem::path sourceDir = std::filesystem::path(output);
    sourceDir /= projectName;
    sourceDir /= "src";
    return sourceDir;
}

std::filesystem::path createHeaderDir(const std::string& output, const std::string& projectName) {
    std::filesystem::path headerDir = std::filesystem::path(output);
    headerDir /= projectName;
    headerDir /= "include";
    headerDir /= projectName;
    return headerDir;
}
} // namespace

Converter::Converter(std::string input, std::string output, std::string projectName)
    : m_input(std::move(input))
    , m_output(std::move(output))
    , m_projectName(std::move(projectName))
{
}

void Converter::convert()
{
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

    std::filesystem::path cppDir{createSourceDir(m_output, m_projectName)};
    std::filesystem::path headerDir{createHeaderDir(m_output, m_projectName)};
    std::filesystem::create_directories(cppDir);
    std::filesystem::create_directories(headerDir);

    for (const auto& rcDialog : rc.dialogs) {
        auto wxDialog = dialogInterpreter::interpret(rcDialog);

        WxEmitter emitter;
        const auto result = emitter.emit(wxDialog);

        std::string outHeaderFile = result.className + ".h";
        {
            std::ofstream hpp(headerDir / outHeaderFile, std::ios::binary);
            if (!hpp) throw std::runtime_error("Could not write " + outHeaderFile);
            hpp << result.header;
        }
        std::string outSourceFile = result.className + ".cpp";
        {
            std::ofstream cpp(cppDir / outSourceFile, std::ios::binary);
            if (!cpp) throw std::runtime_error("Could not write " + outSourceFile);
            cpp << result.source;
        }
        std::cout << "Wrote: " << (cppDir / outHeaderFile).string() << "\n";
        std::cout << "Wrote: " << (cppDir / outSourceFile).string() << "\n";
    }
    std::cout << "Wrote " << rc.dialogs.size() << " dialogs\n";

}

} // namespace wxConvert