#include "ui.h"

#include <array>
#include <cctype>
#include <cstddef>

namespace br {

static std::array<unsigned char, 7> glyph(char c) {
    // A deliberately simple 5x7 pixel alphabet drawn as geometry, no font dependency.
    switch (static_cast<char>(std::toupper(static_cast<unsigned char>(c)))) {
    case 'A': return {14, 17, 17, 31, 17, 17, 17};
    case 'B': return {30, 17, 17, 30, 17, 17, 30};
    case 'C': return {14, 17, 16, 16, 16, 17, 14};
    case 'D': return {30, 17, 17, 17, 17, 17, 30};
    case 'E': return {31, 16, 16, 30, 16, 16, 31};
    case 'F': return {31, 16, 16, 30, 16, 16, 16};
    case 'G': return {14, 17, 16, 23, 17, 17, 15};
    case 'H': return {17, 17, 17, 31, 17, 17, 17};
    case 'I': return {31, 4, 4, 4, 4, 4, 31};
    case 'J': return {7, 2, 2, 2, 18, 18, 12};
    case 'K': return {17, 18, 20, 24, 20, 18, 17};
    case 'L': return {16, 16, 16, 16, 16, 16, 31};
    case 'M': return {17, 27, 21, 21, 17, 17, 17};
    case 'N': return {17, 25, 25, 21, 19, 19, 17};
    case 'O': return {14, 17, 17, 17, 17, 17, 14};
    case 'P': return {30, 17, 17, 30, 16, 16, 16};
    case 'Q': return {14, 17, 17, 17, 21, 18, 13};
    case 'R': return {30, 17, 17, 30, 20, 18, 17};
    case 'S': return {15, 16, 16, 14, 1, 1, 30};
    case 'T': return {31, 4, 4, 4, 4, 4, 4};
    case 'U': return {17, 17, 17, 17, 17, 17, 14};
    case 'V': return {17, 17, 17, 17, 17, 10, 4};
    case 'W': return {17, 17, 17, 21, 21, 21, 10};
    case 'X': return {17, 17, 10, 4, 10, 17, 17};
    case 'Y': return {17, 17, 10, 4, 4, 4, 4};
    case 'Z': return {31, 1, 2, 4, 8, 16, 31};
    case '0': return {14, 17, 19, 21, 25, 17, 14};
    case '1': return {4, 12, 4, 4, 4, 4, 14};
    case '2': return {14, 17, 1, 2, 4, 8, 31};
    case '3': return {30, 1, 1, 14, 1, 1, 30};
    case '4': return {2, 6, 10, 18, 31, 2, 2};
    case '5': return {31, 16, 16, 30, 1, 1, 30};
    case '6': return {14, 16, 16, 30, 17, 17, 14};
    case '7': return {31, 1, 2, 4, 8, 8, 8};
    case '8': return {14, 17, 17, 14, 17, 17, 14};
    case '9': return {14, 17, 17, 15, 1, 1, 14};
    case ':': return {0, 4, 4, 0, 4, 4, 0};
    case '.': return {0, 0, 0, 0, 0, 12, 12};
    case ',': return {0, 0, 0, 0, 0, 4, 8};
    case '-': return {0, 0, 0, 31, 0, 0, 0};
    case '/': return {1, 1, 2, 4, 8, 16, 16};
    case '[': return {14, 8, 8, 8, 8, 8, 14};
    case ']': return {14, 2, 2, 2, 2, 2, 14};
    case '+': return {0, 4, 4, 31, 4, 4, 0};
    case '%': return {25, 26, 2, 4, 8, 11, 19};
    case '?': return {14, 17, 1, 2, 4, 0, 4};
    case '=': return {0, 31, 0, 31, 0, 0, 0};
    case '(': return {2, 4, 8, 8, 8, 4, 2};
    case ')': return {8, 4, 2, 2, 2, 4, 8};
    default: return {0, 0, 0, 0, 0, 0, 0};
    }
}

UI::UI()
    : shader(R"(#version 330 core
layout(location = 0) in vec2 p;
layout(location = 1) in vec4 c;
uniform vec3 uSize;
out vec4 color;

void main() {
    gl_Position = vec4(p.x / uSize.x * 2.0 - 1.0, 1.0 - p.y / uSize.y * 2.0, 0, 1);
    color = c;
})",
R"(#version 330 core
in vec4 color;
out vec4 frag;

void main() {
    frag = color;
})", true) {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Point), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Point),
                          reinterpret_cast<void*>(2 * sizeof(float)));
    glBindVertexArray(0);
}

void UI::rect(float x, float y, float w, float h, Color c) {
    if (w <= 0 || h <= 0) return;
    Point a{x, y, c.r, c.g, c.b, c.a};
    Point b{x + w, y, c.r, c.g, c.b, c.a};
    Point d{x, y + h, c.r, c.g, c.b, c.a};
    Point e{x + w, y + h, c.r, c.g, c.b, c.a};
    vertices.insert(vertices.end(), {a, b, e, a, e, d});
}

void UI::line(float x, float y, float x2, float y2, float width, Color c) {
    float dx = x2 - x;
    float dy = y2 - y;
    float l = std::sqrt(dx * dx + dy * dy);
    if (l < .001f) return;

    // Expand the centre line into two triangles using a perpendicular offset.
    dx = dx / l * width * .5f;
    dy = dy / l * width * .5f;
    Point a{x - dy, y + dx, c.r, c.g, c.b, c.a};
    Point b{x2 - dy, y2 + dx, c.r, c.g, c.b, c.a};
    Point d{x + dy, y - dx, c.r, c.g, c.b, c.a};
    Point e{x2 + dy, y2 - dx, c.r, c.g, c.b, c.a};
    vertices.insert(vertices.end(), {a, b, e, a, e, d});
}

void UI::text(float x, float y, const std::string& s, float scale, Color c) {
    float origin = x;
    for (char a : s) {
        if (a == '\n') {
            y += 10 * scale;
            x = origin;
            continue;
        }

        auto bits = glyph(a);
        for (int row = 0; row < 7; row++) {
            for (int col = 0; col < 5; col++) {
                if (bits[row] & (1 << (4 - col))) {
                    rect(x + col * scale, y + row * scale, scale, scale, c);
                }
            }
        }
        x += 6 * scale;
    }
}

void UI::render(int w, int h) {
    // HUD coordinates start at the top-left. Blend it over the completed scene.
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    shader.use();
    shader.set("uSize", Vec3{static_cast<float>(w), static_cast<float>(h), 0});
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Point), vertices.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
    vertices.clear();
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void UI::destroy() {
    shader.destroy();
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
}

} // namespace br
