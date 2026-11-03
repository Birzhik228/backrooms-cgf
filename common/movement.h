#pragma once
#include "math.h"
#include "balance.h"

namespace br {
// Short acceleration gives each step weight; stronger braking avoids skating.
struct PlayerMotion {
    Vec3 velocity{};
    void reset() { velocity = {}; }
    Vec3 advance(Vec3 desired, float dt) {
        if (dt <= 0) return {};
        const float rate = dot(desired, desired) < dot(velocity, velocity) ||
                           dot(desired, velocity) < 0 ? balance().playerBraking : balance().playerAcceleration;
        const Vec3 change = desired - velocity;
        const float distance = std::sqrt(dot(change, change));
        const Vec3 previous = velocity;
        const float reachTime = distance / rate;
        if (reachTime <= dt) {
            velocity = desired;
            return (previous + desired) * (.5f * reachTime) + desired * (dt - reachTime);
        }
        velocity = velocity + change * (rate * dt / distance);
        return (previous + velocity) * (.5f * dt);
    }
    float speed() const { return std::sqrt(dot(velocity, velocity)); }
};
} // namespace br
