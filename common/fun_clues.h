#pragma once
#include "../world.h"
#include "math.h"
#include <optional>

namespace br {
// A local clue points at a real unopened box, never predicts its seeded roll.
inline std::optional<Present> funPresentClue(const World& world,double x,double z,bool clues) {
    Present gift{};
    if(!clues || !world.isFun() || world.keyCollected() || !world.nearestPresent(x,z,32,gift))return {};
    return gift;
}
inline bool facingPresent(const Present& gift,double x,double z,float eyeHeight,Vec3 forward) {
    const Vec3 delta{float(gift.x-x),.32f-eyeHeight,float(gift.z-z)};
    const float distance=std::sqrt(dot(delta,delta)),direction=std::sqrt(dot(forward,forward));
    return std::isfinite(distance) && std::isfinite(direction) && distance>.01f && direction>.01f &&
           dot(delta,forward)>=.7f*distance*direction;
}
}
