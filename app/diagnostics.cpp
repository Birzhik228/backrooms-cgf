#include "diagnostics.h"

#include <fstream>
#include <iostream>

namespace br::app {
namespace {
std::ofstream logFile;
}

void openLog(const std::filesystem::path& path) {
    logFile.open(path, std::ios::trunc);
}

void log(const std::string& message) {
    std::cout << message << '\n';
    if (logFile) {
        logFile << message << '\n';
        logFile.flush();
    }
}

void glfwError(int code, const char* message) {
    log("GLFW " + std::to_string(code) + ": " + message);
}

} // namespace br::app
