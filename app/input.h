#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "common/math.h"
#include "common/player_state.h"
#include <array>

namespace br::app {

// Owns the input latch and mouse-look filter. Gameplay selects cursor capture
// through pause(); callbacks only collect input and never change game state.
struct Input {
    GLFWwindow* window = nullptr;
    std::array<bool, GLFW_KEY_LAST + 1> pressed{}, pendingPressed{};
    bool paused = true, firstMouse = true;
    double lastX = 0, lastY = 0;
    float yaw = -90, pitch = -3, sensitivity = .095f;
    SmoothLook look;
    bool clicked = false, pendingClick = false;

    void setLook(float y, float p);
    void smooth(float dt);
    void poll();
    bool hit(int key) const;
    bool down(int key) const;
    void onKey(int key, int action);
    void pause(bool value);
    Vec3 forward() const;

    static void key(GLFWwindow* window, int key, int scanCode, int action, int mods);
    static void button(GLFWwindow* window, int button, int action, int mods);
    static void mouse(GLFWwindow* window, double x, double y);
};

} // namespace br::app
