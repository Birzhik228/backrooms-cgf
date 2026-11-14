#include "diagnostics.h"

#include <fstream>
#include <iostream>

namespace br::app {
namespace {
std::ofstream logFile;

void writeLogFile(const std::string& message) {
    if (logFile) {
        logFile << message << '\n';
        logFile.flush();
    }
}
}

void openLog(const std::filesystem::path& path) {
    logFile.open(path, std::ios::trunc);
}

void log(const std::string& message) {
    std::cout << message << std::endl;
    writeLogFile(message);
}

void logError(const std::string& message) {
    std::cerr << message << std::endl;
    writeLogFile(message);
}

void glfwError(int code, const char* message) {
    logError("GLFW " + std::to_string(code) + ": " + message);
}

} // namespace br::app
