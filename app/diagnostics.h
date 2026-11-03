#pragma once

#include <filesystem>
#include <string>

namespace br::app {

void openLog(const std::filesystem::path& path);
void log(const std::string& message);
void glfwError(int code, const char* message);

} // namespace br::app
