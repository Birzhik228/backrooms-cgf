#pragma once

#include <glad/glad.h>
#include <string>
#include <unordered_map>
#include "math.h"

namespace br {

class Shader {
public:
    GLuint id = 0;

    Shader() = default;
    Shader(const std::string& vertex, const std::string& fragment, bool source = false);

    void use() const { glUseProgram(id); }
    GLint location(const char* name) const;

    void set(const char* name, int value) const {
        glUniform1i(location(name), value);
    }
    void set(const char* name, float value) const {
        glUniform1f(location(name), value);
    }
    void set(const char* name, Vec3 value) const {
        glUniform3f(location(name), value.x, value.y, value.z);
    }
    void set(const char* name, const Mat4& value) const {
        glUniformMatrix4fv(location(name), 1, GL_FALSE, value.m);
    }
    void destroy();

private:
    // Locations remain valid for the lifetime of this linked program. Cache -1
    // too, because an optimized-out uniform must not trigger repeated queries.
    mutable std::unordered_map<std::string, GLint> locations_;
};

} // namespace br
