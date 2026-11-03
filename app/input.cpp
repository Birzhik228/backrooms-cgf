#include "input.h"

#include <cmath>

namespace br::app {

void Input::setLook(float y, float p) {
    yaw = y; pitch = p; look.reset(y, p);
}

void Input::smooth(float dt) {
    if (!paused) {
        look.update(dt); yaw = look.yaw; pitch = look.pitch;
    }
}

void Input::poll() {
    // A press followed by a release within one frame must still toggle the
    // action. The callback latches press events until the next frame consumes them.
    pressed = pendingPressed;
    pendingPressed.fill(false);
    clicked = pendingClick; pendingClick = false;
}

bool Input::hit(int key) const { return pressed[key]; }
bool Input::down(int key) const { return glfwGetKey(window, key) == GLFW_PRESS; }

void Input::onKey(int key, int action) {
    if (key >= 0 && key <= GLFW_KEY_LAST && action == GLFW_PRESS) pendingPressed[key] = true;
}

void Input::key(GLFWwindow* window, int key, int, int action, int) {
    auto* self = static_cast<Input*>(glfwGetWindowUserPointer(window));
    if (self) self->onKey(key, action);
}

void Input::button(GLFWwindow* window, int button, int action, int) {
    auto* self = static_cast<Input*>(glfwGetWindowUserPointer(window));
    if (self && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) self->pendingClick = true;
}

void Input::pause(bool value) {
    paused = value;
    firstMouse = true;
    look.reset(yaw, pitch);
    glfwSetInputMode(window, GLFW_CURSOR,
                     value ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

Vec3 Input::forward() const {
    const float y = yaw * PI / 180, p = pitch * PI / 180;
    return normalize({std::cos(y) * std::cos(p), std::sin(p),
                      std::sin(y) * std::cos(p)});
}

void Input::mouse(GLFWwindow* window, double x, double y) {
    auto* self = static_cast<Input*>(glfwGetWindowUserPointer(window));
    if (!self || self->paused) return;
    if (self->firstMouse) {
        self->lastX = x;
        self->lastY = y;
        self->firstMouse = false;
    }
    self->look.add(float(x - self->lastX), float(y - self->lastY), self->sensitivity);
    self->lastX = x;
    self->lastY = y;
}

} // namespace br::app
