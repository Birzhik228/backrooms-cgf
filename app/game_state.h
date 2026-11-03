#pragma once

#include "world.h"
#include "common/movement.h"
#include "common/player_state.h"

namespace br::app {

// Data shared by movement, interactions, the camera and the HUD. Resets stay
// explicit in Game::resetPlayer(): returning to the entrance and starting a new
// level intentionally preserve different parts of the session.
struct PlayerState {
    double x, z;
    PlayerMotion motion{};
    Stamina stamina{};
    bool hiding = false;
    HidingSpot hidingSpot{};
    bool crouched = false;
    float eyeHeight = 1.65f;
    float bobOffset = 0;
    double walkDistance = 0, travelDistance = 0;
    float minimumStamina = 100;
};

// Wall-clock timestamps and phase durations; kept separate from simulation dt.
struct FrameTimings {
    double frameStart = 0, rawDelta = 0;
    double queueWaitMs = 0;
    double inputStart = 0, inputMs = 0;
    double moveStart = 0, moveMs = 0;
    double worldMs = 0, audioMs = 0, uploadMs = 0;
    double setupMs = 0, sceneMs = 0, lensMs = 0;
    double uiStart = 0;
};

} // namespace br::app
