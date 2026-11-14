#include "game_internal.h"

namespace br::app {

void Game::startPresentation() {
    // A deliberate fresh scene, using the normal world and rendering passes.
    // Presentation values never overwrite the player's saved Settings.
    replaceWorld(PRESENTATION_SEED, Level::Zero);
    presentationActive = true;
    presentationAO = PRESENTATION_AO;
    input.setLook(PRESENTATION_YAW, PRESENTATION_PITCH);
    forward = input.forward();
    player.bobOffset = 0;
    moving = sprinting = false;
    frameDistance = 0;
    fieldOfView = PRESENTATION_FOV;
    flashlight = true;
    clues = showMap = diagnostics = cameraView = wireframe = false;
    elapsed = 1.5;
    flow.guidePending = false;
    flow.entered = true;
    flow.escaped = false;
    exitUsed = false;
    doorsUsed = 0;
    notice.clear();
    noticeTime = 0;
    menuSelection = 0;
    setScreen(Screen::Gameplay);
    log("Presentation started: fresh Level 0 / seed " + std::to_string(PRESENTATION_SEED) +
        " / AO ON / gameplay frozen / guide skipped");
}

void Game::endPresentation() {
    if (!presentationActive) return;
    leaveInspectionCamera();
    presentationActive = false;
    fieldOfView = settings.fov;
    log("Presentation ended: normal play and graphics settings restored");
}

} // namespace br::app
