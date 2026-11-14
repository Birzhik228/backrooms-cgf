#include "game_internal.h"

namespace br::app {

float Game::activeAOAmount() const {
    return encounter.presenting() || wireframe ? 0.f :
        presentationActive ? presentationAO : settings.ambientOcclusion;
}

void Game::toggleAmbientOcclusion() {
    if (presentationActive) {
        presentationAO = presentationAO > 0 ? 0.f : PRESENTATION_AO;
        log(std::string("AO: ") + (presentationAO > 0 ? "ON" : "OFF") +
            " / presentation amount " + std::to_string(presentationAO));
        return;
    }
    if (settings.ambientOcclusion > 0) {
        lastAOAmount = settings.ambientOcclusion;
        settings.ambientOcclusion = 0;
    } else {
        settings.ambientOcclusion = lastAOAmount;
    }
    log(std::string("AO: ") + (settings.ambientOcclusion > 0 ? "ON" : "OFF") +
        " / amount " + std::to_string(settings.ambientOcclusion));
}

void Game::toggleInspectionCamera() {
    if (inspectionCamera.active) {
        leaveInspectionCamera();
        return;
    }
    // Streaming discards shifted chunk variants. Preserve the complete bounded
    // gameplay world so returning cannot change the player's collision layout.
    inspectionWorldSnapshot = world;
    inspectionCamera.active = true;
    inspectionCamera.x = player.x;
    inspectionCamera.z = player.z;
    inspectionCamera.y = player.eyeHeight + player.bobOffset;
    inspectionCamera.playerYaw = input.yaw;
    inspectionCamera.playerPitch = input.pitch;
    player.motion.reset();
    moving = sprinting = false;
    frameDistance = 0;
    log("Inspection camera: ON / gameplay frozen");
}

void Game::leaveInspectionCamera() {
    if (!inspectionCamera.active) return;
    inspectionCamera.active = false;
    world = std::move(*inspectionWorldSnapshot);
    inspectionWorldSnapshot.reset();
    gpu.clear();
    for (auto& cached : shadows.cache) cached.valid = false;
    input.setLook(inspectionCamera.playerYaw, inspectionCamera.playerPitch);
    forward = input.forward();
    // Restore collision geometry before normal player/actor updates resume.
    world.update(player.x, player.z);
    player.motion.reset();
    doorVisualDirty = true;
    log("Inspection camera: OFF / player view restored");
}

void Game::updateInspectionCamera() {
    forward = input.forward();
    moving = sprinting = false;
    frameDistance = 0;
    if (input.paused) return;
    const Vec3 axes = inspectionMovementInput();
    const Vec3 right = normalize(cross(forward, {0, 1, 0}));
    Vec3 direction = right * axes.x + Vec3{0, axes.y, 0} + forward * axes.z;
    const float magnitude = std::sqrt(dot(direction, direction));
    if (magnitude > 1) direction = direction * (1.f / magnitude);
    const Vec3 delta = direction * ((inspectionMoveFast() ? 20.f : 5.f) * dt);
    inspectionCamera.x += delta.x;
    inspectionCamera.y += delta.y;
    inspectionCamera.z += delta.z;
}

} // namespace br::app
