#pragma once

namespace br::app {

// Opt-in checks compare live presentation state across completed frames.
struct PresentationProbe {
    bool captured = false, verified = false;
    double playerX = 0, playerZ = 0, elapsed = 0, threatX = 0, threatZ = 0;
    float stamina = 0, bob = 0, animation = 0;
    unsigned aoPasses = 0;
};

} // namespace br::app
