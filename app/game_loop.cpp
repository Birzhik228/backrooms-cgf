#include "game_internal.h"

namespace br::app {
namespace fs = std::filesystem;

int Game::execute() {
    initialize();
    while (!glfwWindowShouldClose(window)) {
        if(options.runSeconds>0 && glfwGetTime()-runStart>=options.runSeconds)break;
        timing.frameStart = glfwGetTime();
        if (!options.unboundedQueue) frameQueue.wait();
        timing.queueWaitMs = (glfwGetTime() - timing.frameStart) * 1000;
        timing.rawDelta = std::max(0.0, timing.frameStart - lastTime);
        lastTime = timing.frameStart;
        // Cap ordinary simulation stalls to avoid teleporting through a wall.
        // The automated walk has deterministic motion per frame, independent of
        // render speed; performance counters always use real wall-clock time.
        dt = usesFixedTimeStep() ? 1.f / 60.f :
                         static_cast<float>(std::min(timing.rawDelta, .05));
        elapsed += dt;
        timing.inputStart = glfwGetTime();
        glfwPollEvents();
        applyScenarioInput();
        if(input.hit(GLFW_KEY_F9) && !flow.cinematic() && !encounter.scaring())startEnding("preview");
        if(flow.cinematic() && renderEnding())continue;
        timing.inputMs = (glfwGetTime() - timing.inputStart) * 1000;
        timing.moveStart = glfwGetTime();
        handleMenus();
        if(!updateGameplay())continue;
        updateAudio();
        if(!renderWorld())continue;
        drawHud();
        if(!finishFrame())break;
    }
    reportAndShutdown();
    return 0;
}

bool Game::finishFrame() {
    // Capture before swapping, while the completed image is in GL_BACK.
    if (input.hit(GLFW_KEY_F12)) {
        const auto stamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        screenshot(fs::path("captures") / ("backrooms-" + std::to_string(stamp) + ".bmp"),
                   width, height);
        notice = "SCREENSHOT SAVED IN CAPTURES";
        noticeTime = 3;
    }
    const bool lastFrame = options.frames > 0 && frame + 1 >= options.frames;
    if (lastFrame && !options.capture.empty()) {
        screenshot(options.capture, width, height);
    }
    const GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        throw std::runtime_error("OpenGL error " + std::to_string(error) +
                                 " at frame " + std::to_string(frame));
    }
    const double uiMs = (glfwGetTime() - timing.uiStart) * 1000;
    if (!options.unboundedQueue) frameQueue.submit();
    const double swapStart = glfwGetTime();
    glfwSwapBuffers(window);
    const double swapMs = (glfwGetTime() - swapStart) * 1000;
    int cap = options.fpsLimit >= 0 ? options.fpsLimit : options.hidden ? 0 : settings.maxFps;
    if (!options.hidden && (input.paused || !glfwGetWindowAttrib(window, GLFW_FOCUSED)))
        cap = cap > 0 ? std::min(cap, 30) : 30;
    const double workMs=(glfwGetTime()-timing.frameStart)*1000;
    // Sleep only the remaining frame budget: no busy-wait and no catch-up burst.
    // Swap/vsync and GPU queue waits already count towards this budget.
    if (cap > 0) {
        const double remaining = 1.0 / cap - (glfwGetTime() - timing.frameStart);
        framePacer.wait(remaining);
    }
    ++frame;

    // Measure complete rendered frames, including swap/vsync. These real
    // durations are never clamped to the physics time-step limit.
    const double frameSeconds = glfwGetTime() - timing.frameStart;
    maxFrameMs = std::max(maxFrameMs, frameSeconds * 1000);
    performance.record(frame,frameSeconds*1000,workMs,cap>0?1000.0/cap:0,
        int(world.level()),false,world.chunks().size(),world.preparedChunks().size(),gpu.size());
    if (profile) profile << frame << "," << player.x << "," << player.z << "," << origin.x << "," << origin.z << ","
                         << timing.worldMs << "," << timing.uploadMs << "," << frameSeconds * 1000 << "," << uploads << "," << lightCount << "," << timing.sceneMs << "," << timing.lensMs << "," << uiMs << "," << swapMs << "," << timing.inputMs << "," << timing.moveMs << "," << timing.audioMs << "," << timing.setupMs << "," << timing.queueWaitMs << "," << residentTriangles << "," << drawnTriangles
        << "," << encounter.elapsed() << "," << encounter.cameraTurn()
        << "," << (1.65f+(scareConfig.faceDistance-1.65f)*(scareConfig.lunge?encounter.lunge():0))
        << "," << renderFov << "," << scareShake.x << "," << scareShake.y << "," << scareShake.z << "," << shakeRoll
        << "," << cameraForward.x << "," << cameraForward.y << "," << cameraForward.z
        << "," << threat.x() << "," << threat.z() << "," << threatBodyYaw << "," << threatHeadYaw
        << "," << (std::hypot(threat.x()-oldThreatX,threat.z()-oldThreatZ)>.00001?
            ((threat.x()-oldThreatX)*std::sin(threatBodyYaw)-(threat.z()-oldThreatZ)*std::cos(threatBodyYaw))/std::hypot(threat.x()-oldThreatX,threat.z()-oldThreatZ):1.) << "\n";
    fpsTime += frameSeconds;
    ++fpsFrames;
    if (fpsTime >= .5) {
        fps = static_cast<float>(fpsFrames / fpsTime);
        fpsTime = 0;
        fpsFrames = 0;
    } else if (frame == 1 && frameSeconds > 0) {
        fps = static_cast<float>(1.0 / frameSeconds);
    }
    if (lastFrame) {
        return false;
    }
    return true;
}

void Game::reportAndShutdown() {
    const double runtime = glfwGetTime() - runStart;
    const double averageFps = runtime > 0 ? frame / runtime : 0;
    std::ostringstream summary;
    summary << std::fixed << std::setprecision(2)
            << "Completed " << frame << " frames. Max resident chunks: " << maxChunks
            << ". Average FPS: " << averageFps << ". Last FPS: " << fps
            << ". Max frame ms: " << maxFrameMs << ". Position: " << player.x << ", " << player.z
            << ". Shifts: " << world.shifts() << ". Room: " << roomName(roomInfo(int64_t(std::floor(player.x/CELL)),int64_t(std::floor(player.z/CELL)),world.seed(),world.level()).kind)
            << ". Exit used: " << (exitUsed ? "yes" : "no")
            << ". Escaped: " << (flow.escaped ? "yes" : "no")
            << ". Circuits: " << world.breakersPowered() << "/3"
            << ". Circuit states: " << (world.isParking()?
                std::string("A ")+(world.breakerPowered(0)?"ON":"OFF")+" / B "+(world.breakerPowered(1)?"ON":"OFF")+" / C "+(world.breakerPowered(2)?"ON":"OFF"):"inactive")
            << ". Circuit target: " << (parkingCircuitTarget(world,player.x,player.z)>=0?std::string(1,char('A'+parkingCircuitTarget(world,player.x,player.z))):"none")
            << ". Level: " << levelName(world.level()) << ". Mouse active: " << (mouse.active?"yes":"no")
            << ". Eye height: " << player.eyeHeight << ". Crouched: " << (player.crouched ? "yes" : "no")
            << ". FOV: " << fieldOfView << ". Velocity: " << player.motion.speed()
            << ". Verity: " << (world.infiniteStamina()?"active":"inactive") << ". Stamina: " << player.stamina.value << ". Minimum stamina: " << player.minimumStamina
            << ". Doors used: " << doorsUsed << ". Tension: " << tension
            << ". Creature footfalls: " << threatFootfalls
            << ". Creature walk/run footfalls: " << threatWalkFootfalls << "/" << threatRunFootfalls
            << ". Creature visual notices/roars: " << threatNotices << "/" << threatRoars
            << ". Level 0 ambience/steps/hints: " << atmosphereAmbient << "/" << atmosphereSteps << "/" << atmosphereHints
            << ". Level 0 notes read: " << observationsRead << ". Note visible: " << (observationTime>0&&!input.paused&&encounter.alive()&&!cameraView?"yes":"no")
            << ". Level 0 door hints: " << presenceEvents << ". Door hint active: " << (presence.doorId()?"yes":"no")
            << ". Fun steps/balloons/melodies: " << partyStepEvents << "/" << funBalloonEvents << "/" << funMelodyEvents
            << ". Frozen Partygoer steps: " << partyFrozenSteps
            << ". Parking pipes/drips/vents/relays: " << parkingPipeEvents << "/" << parkingDripEvents << "/" << parkingVentEvents << "/" << parkingRelayEvents
            << ". Head yaw: " << threatHeadYaw*180/PI
            << ". Threat: " << threatStateName(threat.state()) << ". Catches: " << catches << ". Restarts: " << restarts
            << ". Mouse squeaks: " << mouseSqueaks << ". Guide: " << (flow.is(Screen::Controls)?"open":"closed")
            << ". Guide page: " << guidePage+1
            << ". Level menu: " << (flow.is(Screen::LevelSelect)?"open":"closed") << ". Level menu opens/travels: " << levelMenuVisits << "/" << levelMenuTravel
            << ". Presents opened: " << world.presentsOpened() << ". Key: " << (world.keyCollected()?"collected":"missing") << ". Hiding: " << (player.hiding?"yes":"no")
            << ". Present clue: " << (funPresentClue(world,player.x,player.z,clues)&&!input.paused&&!cameraView?"visible":"off")
            << ". Encounter: " << (encounter.scaring()?"jumpscare":encounter.gameOver()?"game-over":"exploring")
            << ". Music loaded: " << (musicLoaded ? "yes" : "no")
            << ". Music enabled: " << (settings.musicEnabled ? "yes" : "no")
            << ". Brightness: " << settings.brightness << ". Volume: " << settings.volume
            << ". Paused: " << (input.paused ? "yes" : "no") << ". GL errors: 0.";
    log(summary.str());
    log("Streaming: max prepared chunks " + std::to_string(maxPrepared) +
        ", cached promotions " + std::to_string(world.streamPromotions()) +
        ", synchronous fallback chunks " + std::to_string(world.streamFallbacks()) + ".");

    // Session summary is emitted before automatic resource destruction.
    log("Ending time: "+std::to_string(ending.time())+" / active "+(flow.cinematic()?"yes":"no")+
        " / completed "+std::to_string(endingCompletions)+" / replayed "+std::to_string(endingReplays));
    log(std::string("Ending gameplay frozen: ")+(endingWorldFrozen?"yes":"no"));
    log("Lamp shadow cache updates: "+std::to_string(lampShadowUpdates));
    audio.stop();
    log("Partygoers: "+std::to_string(partygoers.activeCount())+" / frozen frames "+std::to_string(partygoers.frozenFrames)+" / travelled "+std::to_string(partygoers.travelled));
    log("Darkness eyes: "+std::to_string(darkness.activeCount())+" / dispersed "+std::to_string(darkness.dispelled)+" / spawned "+std::to_string(darkness.spawned));
    music.stop();
    performance.mark("session-end",int(world.level()),world.chunks().size(),world.preparedChunks().size(),gpu.size());
    log(performance.summary());
    log("Stress transitions: "+std::to_string(stressTransitions));
    // GL members now release automatically before WindowContext is destroyed.

}

} // namespace br::app
