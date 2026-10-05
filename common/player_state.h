#pragma once
#include <algorithm>
#include <cmath>

namespace br {
struct Stamina {
    float value = 100.f, recoveryDelay = 0.f;
    bool exhausted = false;
    void reset() { *this = {}; }
    bool canSprint() const { return !exhausted && value > 0; }
    void update(float dt, bool sprintTravel, bool paused) {
        if (paused || !std::isfinite(dt) || dt <= 0) return;
        if (sprintTravel) {
            value = std::max(0.f, value - 18.f * dt);
            recoveryDelay = 1.5f;
            if (value == 0) exhausted = true;
        } else {
            const float recovering = std::max(0.f, dt - recoveryDelay);
            recoveryDelay = std::max(0.f, recoveryDelay - dt);
            value = std::min(100.f, value + 14.f * recovering);
            if (value >= 22.f) exhausted = false;
        }
    }
};

// Exponential smoothing uses seconds, keeping the response independent of FPS.
struct SmoothLook {
    float yaw = -90, pitch = -3, targetYaw = -90, targetPitch = -3;
    void reset(float y, float p) { yaw = targetYaw = y; pitch = targetPitch = p; }
    void add(float dx, float dy, float sensitivity) {
        targetYaw += dx * sensitivity;
        targetPitch = std::clamp(targetPitch - dy * sensitivity, -85.f, 85.f);
    }
    void update(float dt) {
        const float blend = 1.f - std::exp(-std::max(0.f, dt) / .018f);
        yaw += (targetYaw - yaw) * blend;
        pitch += (targetPitch - pitch) * blend;
        if (std::abs(yaw) > 36000.f) {
            const float turns = std::floor(yaw / 360.f) * 360.f;
            yaw -= turns; targetYaw -= turns;
        }
    }
};
}
