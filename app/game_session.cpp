#include "game_internal.h"

namespace br::app {
void Game::setScreen(Screen screen) {
    // Screen selection is the sole writer of the input pause/cursor mode.
    // Capture remains in Encounter; cinematics stay paused for gameplay even
    // while their own timeline is running. Historical run metadata is intact.
    flow.open(screen);
    input.pause(flow.capturesInput());
}

void Game::resetPlayer() {
    // A return to the entrance resets actors and transient audio, while keeping
    // the current world's collected items, opened doors and powered circuits.
    px = pz = CELL*.5;
    input.setLook(-90, -3);
    world.update(px, pz);
    flow.escaped = false; exitArmed=true; doorVisualDirty=true;
    motion.reset();
    bobOffset = 0;
    crouched = false;
    eyeHeight = 1.65f;
    fieldOfView = settings.fov;
    stamina.reset(); tension = 0;
    audio.reset(static_cast<uint32_t>(world.seed()));
    threat.reset(world.seed()); threat.setEnraged(world.keyCollected()); encounter.reset();
    mouse.reset(world.seed());mouseSqueaks=0;presentMouseTime=0;
    partygoers.reset();release(partyGpu);lastPartyPose={};
    darkness.reset();release(darknessGpu);
    hiding=false; threatVocals.reset(); occlusionClock=0; threatTransmission=1;
    atmosphere.reset(world.seed());
    funAudio.reset(world.seed());
    parkingAudio.reset(world.seed());
    presence.reset(world.seed());observationTime=0;observationKind=-1;
    threatAnimation=0; lastThreatMeshTime=-1;
    threatStoop=0; lastThreatStoop=-1; threatRunningSteps=false;
    threatGaitPhase=threatVisualSpeed=threatHeadYaw=threatHeadPitch=threatTurnWeight=0; lastThreatContact={};
    threatFootPlanting.reset();
    threatBodyYaw=threat.yaw();
    travel = walk = shiftClock = 0;
    audio.update(0, 0, false, false, Surface::Carpet, true);
    notice = "RETURNED TO THE FIRST ROOM";
    noticeTime = 3;
    log("Returned to the first room.");
}

void Game::replaceWorld(uint64_t seed, Level level) {
    // Build the new CPU world first, then discard render data belonging to the
    // old one. Reset the player/actors/audio once, after invalidating shadows;
    // resetPlayer() prepares the entrance chunks in the replacement world.
    performance.mark("level-leaving",int(world.level()),world.chunks().size(),world.preparedChunks().size(),gpu.size());
    world=World(seed,level);
    gpu.clear(); // Move-only chunk owners release their buffers and VAOs.
    release(doorGpu);
    release(keyGpu);
    release(mouseGpu);
    for (auto& cached:shadows.cache) cached.valid=false;
    resetPlayer();
    performance.mark("level-ready",int(world.level()),world.chunks().size(),world.preparedChunks().size(),gpu.size());
}

void Game::freshRun(uint64_t seed) {
    guidePage=0;flow.guidePending=(!options.hidden && !options.tour) || options.guideCheck;
    replaceWorld(seed,world.level());
    exitUsed=false; doorsUsed=0; minimumStamina=100;
    notice.clear(); noticeTime=0; menuSelection=0;
    setScreen(Screen::Gameplay); flow.entered=true;
}

void Game::enterLevel(Level next,bool selected) {
    const Level previousLevel=world.level();
    const auto releasedMeshes=gpu.size();
    replaceWorld(world.seed(),next);
    log("Level cleanup: released "+std::to_string(releasedMeshes)+" chunk meshes; old GPU chunks remaining "+std::to_string(gpu.size()));
    exitUsed=!selected;exitArmed=selected;flow.escaped=false;
    flow.guidePending=false;flow.entered=true;
    setScreen(Screen::Gameplay);
    if(selected) {
        flashlight=true;showMap=false;doorsUsed=0;minimumStamina=100;
        ++levelMenuTravel;
        log(std::string("Level selected: ")+levelName(next)+" / fresh entrance / seed "+std::to_string(world.seed()));
    }
    notice=world.isParking()?"PARKING / SHINE YOUR LIGHT AT THE EYES / RESTORE POWER":world.isFun()?
        "LEVEL FUN / CHECKPOINT REACHED / SEARCH THE PRESENTS":"LEVEL 0 / FIND THE KEY / REACH THE EXIT";noticeTime=8;
    if(!selected && previousLevel==Level::Fun && world.isParking()) {
        log("Checkpoint reached: Parking entrance.");
        log("Transition: Level Fun -> Parking. Restore circuits A, B and C.");
    } else if(!selected && previousLevel==Level::Zero && world.isFun()) {
        log("Checkpoint reached: Level Fun entrance.");
        log("Transition: Level 0 -> Level Fun. Partygoers enabled; Aranea and mouse disabled.");
    }
}


} // namespace br::app
