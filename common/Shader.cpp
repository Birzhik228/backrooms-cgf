#include "Shader.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace br {
namespace {

std::string read(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Cannot read shader: " + path + ". Start from the project folder.");
    }
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

GLuint compile(GLenum type, const std::string& text) {
    const GLuint shader = glCreateShader(type);
    const char* source = text.c_str();
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> log(length + 1);
        glGetShaderInfoLog(shader, length, nullptr, log.data());
        glDeleteShader(shader);
        throw std::runtime_error(std::string("Shader compile failed: ") + log.data());
    }
    return shader;
}

} // namespace

Shader::Shader(const std::string& vertex, const std::string& fragment, bool source) {
    const GLuint vs = compile(GL_VERTEX_SHADER, source ? vertex : read(vertex));
    GLuint fs = 0;
    try {
        fs = compile(GL_FRAGMENT_SHADER, source ? fragment : read(fragment));
    } catch (...) {
        glDeleteShader(vs);
        throw;
    }

    id = glCreateProgram();
    glAttachShader(id, vs);
    glAttachShader(id, fs);
    glLinkProgram(id);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint success = 0;
    glGetProgramiv(id, GL_LINK_STATUS, &success);
    if (!success) {
        GLint length = 0;
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &length);
        std::vector<char> log(length + 1);
        glGetProgramInfoLog(id, length, nullptr, log.data());
        destroy();
        throw std::runtime_error(std::string("Shader link failed: ") + log.data());
    }
}

GLint Shader::location(const char* name) const {
    const auto found = locations_.find(name);
    if (found != locations_.end()) {
        return found->second;
    }
    const GLint result = glGetUniformLocation(id, name);
    locations_.emplace(name, result);
    return result;
}

void Shader::destroy() {
    if (id) {
        glDeleteProgram(id);
    }
    id = 0;
    locations_.clear();
}

} // namespace br
