#include "options.h"
#include "common/ending.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace br::app {

Options parseOptions(int argc, char** argv) {
    Options options;
    const auto real=[](const std::string& token) {
        std::size_t used=0;const double value=std::stod(token,&used);
        if(used!=token.size())throw std::runtime_error("Invalid numeric option: "+token);
        return value;
    };
    const auto integer=[](const std::string& token) {
        std::size_t used=0;const int value=std::stoi(token,&used);
        if(used!=token.size())throw std::runtime_error("Invalid integer option: "+token);
        return value;
    };
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) {
                throw std::runtime_error("Missing argument for " + argument);
            }
            return argv[++i];
        };

        if (argument == "--performance-report") {
            options.performanceReport=next();
        } else if(argument=="--balance-file") {
            options.balanceFile=next();
        } else if(argument=="--slow-frame-ms") {
            options.slowFrameMs=real(next());
        } else if(argument=="--run-seconds") {
            options.runSeconds=real(next());
        } else if(argument=="--stress-cycles") {
            options.stressCycles=integer(next());
        } else if(argument=="--stress-interval") {
            options.stressInterval=integer(next());
        } else if (argument == "--seed") {
            options.seed = std::stoull(next());options.explicitSeed=true;
        } else if (argument == "--frames") {
            options.frames = std::stoi(next());
        } else if (argument == "--width") {
            options.width = std::stoi(next());
        } else if (argument == "--height") {
            options.height = std::stoi(next());
        } else if (argument == "--x") {
            options.spawnX = std::stod(next());
        } else if (argument == "--z") {
            options.spawnZ = std::stod(next());
        } else if (argument == "--yaw") {
            options.yaw = std::stof(next());
        } else if (argument == "--pitch") {
            options.pitch = std::stof(next());options.explicitPitch=true;
        } else if (argument == "--capture") {
            options.capture = next();
        } else if (argument == "--bounded-queue") {
            options.unboundedQueue = false;
        } else if (argument == "--unbounded-queue") {
            options.unboundedQueue = true;
        } else if (argument == "--jumpscare-config") {
            options.jumpscareConfigFile=next();
        } else if (argument == "--profile") {
            options.profile = next();
        } else if (argument == "--fps-limit") {
            options.fpsLimit = std::clamp(std::stoi(next()), 0, 240);
            if(options.fpsLimit>0) options.fpsLimit=std::max(30,options.fpsLimit);
        } else if (argument == "--ending-preview") {
            options.endingPreview=true;
        } else if (argument == "--ending-time") {
            options.endingPreview=true;options.endingTime=std::stof(next());
            if(!std::isfinite(options.endingTime))throw std::runtime_error("Invalid ending time");
            options.endingTime=std::clamp(options.endingTime,0.f,EndingSequence::DURATION);
        } else if (argument == "--ending-check") {
            options.endingPreview=true;options.endingCheck=true;
        } else if (argument == "--hidden") {
            options.hidden = true;
        } else if (argument == "--demo") {
            options.demo = true;
        } else if (argument == "--tour") {
            options.tour = true;
        } else if (argument == "--no-flashlight") {
            options.noFlashlight = true;
        } else if (argument == "--no-clues") {
            options.noClues = true;
        } else if (argument == "--map") {
            options.map = true;
        } else if (argument == "--metrics") {
            options.metrics = true;
        } else if (argument == "--wireframe") {
            options.wireframe = true;
        } else if (argument == "--auto-walk") {
            options.autoWalk = true;
        } else if (argument == "--auto-exit") {
            options.autoExit = true;
        } else if (argument == "--auto-continue") {
            options.autoContinue = true;
        } else if (argument == "--auto-crouch") {
            options.autoCrouch = true;
        } else if (argument == "--auto-stand") {
            options.autoStand = true;
        } else if (argument == "--auto-sprint") {
            options.autoSprint = true;
        } else if (argument == "--auto-stop") {
            options.autoStop = true;
        } else if (argument == "--auto-release-sprint") {
            options.autoReleaseSprint = true;
        } else if (argument == "--camera-view") {
            options.cameraView = true;
        } else if (argument == "--settings") {
            options.settingsMenu = true;
        } else if (argument == "--auto-settings") {
            options.settingsMenu = options.autoSettings = true;
        } else if (argument == "--settings-file") {
            options.settingsFile = next();
        } else if (argument == "--music-file") {
            options.musicFile = next();
        } else if (argument == "--music-menu-check") {
            options.settingsMenu = options.musicMenuCheck = true;
        } else if (argument == "--door-close-frame") {
            options.doorCloseFrame = std::stoi(next());
        } else if (argument == "--menu-check") {
            options.menuCheck = true;
        } else if (argument == "--auto-pause") {
            options.autoPause = true;
        } else if (argument == "--auto-door") {
            options.autoDoor = true;
        } else if (argument == "--auto-close-door") {
            options.autoDoor = options.autoCloseDoor = true;
        } else if (argument == "--no-threat") { options.noThreat=true;
        } else if (argument == "--partygoer-preview") { options.partyPreview=true; options.funPreview=true;
        } else if (argument == "--partygoer-check") { options.partyPreview=true;options.partyCheck=true;options.funPreview=true;
        } else if (argument == "--fun-ambience-preview") { options.funAmbiencePreview=true;
        } else if (argument == "--exit-preview") { options.exitPreview=true;
        } else if (argument == "--key-preview") { options.keyPreview=true;
        } else if (argument == "--level-fun") { options.funPreview=true;
        } else if (argument == "--parking") { options.parkingPreview=true;
        } else if (argument == "--parking-ambience-preview") { options.parkingAmbiencePreview=true;
        } else if (argument == "--auto-breakers") { options.autoBreakers=true;
        } else if (argument == "--darkness-preview") { options.parkingPreview=true;options.darknessPreview=true;
        } else if (argument == "--threat-preview") { options.threatPreview=true; options.forcedThreat=true;
        } else if (argument == "--threat-x") { options.threatX=std::stod(next()); options.forcedThreat=true;
        } else if (argument == "--threat-z") { options.threatZ=std::stod(next()); options.forcedThreat=true;
        } else if (argument == "--threat-yaw") { options.threatYaw=std::stof(next()); options.forcedThreat=true;
        } else if (argument == "--threat-time") { options.threatPreviewTime=std::stof(next());
        } else if (argument == "--threat-speed") { options.threatPreviewSpeed=std::stof(next());
        } else if (argument == "--threat-head-yaw") { options.threatPreviewHeadYaw=std::stof(next());
        } else if (argument == "--threat-head-pitch") { options.threatPreviewHeadPitch=std::stof(next());
        } else if (argument == "--auto-catch") { options.autoCatch=true;
        } else if (argument == "--auto-restart") { options.autoRestart=true;
        } else if (argument == "--auto-restart-click") { options.autoRestartClick=true;
        } else if (argument == "--auto-scare-escape") { options.autoScareEscape=true;
        } else if (argument == "--auto-verity") { options.autoVerity=true;options.autoKey=true;options.funPreview=true;
        } else if (argument == "--auto-key") { options.autoKey=true;
        } else if (argument == "--mouse-fixture") { options.mouseFixture=true;
        } else if (argument == "--mouse-preview") { options.mousePreview=true;
        } else if (argument == "--guide-check") { options.guideCheck=true;
        } else if (argument == "--guide-close") { options.guideClose=true;
        } else if (argument == "--guide-click") { options.guideClick=true;
        } else if (argument == "--guide-back") { options.guideBack=true;
        } else if (argument == "--guide-skip") { options.guideSkip=true;
        } else if (argument == "--level-menu-check") { options.levelMenuCheck=true;
        } else if (argument == "--level-menu-close") { options.levelMenuCheck=true;options.levelMenuClose=true;
        } else if (argument == "--level-menu-click") { options.levelMenuCheck=true;options.levelMenuClick=true;
        } else if (argument == "--level-menu-resume") { options.levelMenuCheck=true;options.levelMenuResume=true;
        } else if (argument == "--level-menu-back-click") { options.levelMenuCheck=true;options.levelMenuBackClick=true;
        } else if (argument == "--level-menu-at") {
            options.levelMenuAt=std::clamp(std::stoi(next()),0,10000);
        } else if (argument == "--level-menu-select") {
            options.levelMenuCheck=true;options.levelMenuTarget=std::stoi(next());
            if(options.levelMenuTarget<1||options.levelMenuTarget>3)throw std::runtime_error("Level selection must be 1, 2 or 3.");
        } else if (argument == "--control-hold-check") { options.controlHoldCheck=true;
        } else if (argument == "--control-pause-check") { options.controlPauseCheck=true;
        } else if (argument == "--control-tap-check") { options.controlTapCheck=true;
        } else if (argument == "--observation-preview") {
            options.observationPreview=std::clamp(std::stoi(next()),0,1);
        } else if (argument == "--auto-read-observation") {
            options.autoReadObservation=true;
            if(options.observationPreview<0)options.observationPreview=0;
        } else if (argument == "--presence-preview") {
            options.presencePreview=true;options.threatPreview=true;options.forcedThreat=true;
        } else if (argument == "--auto-hide") { options.autoHide=true;
        } else if (argument == "--hide-fixture") { options.hideFixture=true;
        } else if (argument == "--auto-jumpscare") { options.autoJumpscare=true;
        } else if (argument == "--audio-preview") {
            options.audioPreview = next();
        } else if (argument == "--help") {
            std::cout << "Backrooms: --seed N --frames N --capture image.bmp --hidden "
                         "--demo --tour --x X --z Z --yaw DEGREES --width N --height N\n"
                         "--level-fun starts the party checkpoint; --parking starts the third stage.\n"
                         "Escape opens the pause menu with level selection; hold Ctrl to crouch.\n"
                         "--observation-preview 0|1 / --auto-read-observation inspect optional Level 0 notes.\n"
                         "--presence-preview uses the specified player/creature pose to verify a real-door hint.\n"
                         "--level-menu-check [--level-menu-select 1..3] [--level-menu-click] verifies the selector.\n"
                         "--level-menu-close / --control-hold-check verify closing and crouch input.\n"
                         "--darkness-preview / --auto-breakers verify garage threats and power.\n"
                         "Verification: --exit-preview --key-preview --no-flashlight --map --metrics --wireframe --auto-walk\n"
                         "--auto-exit simulates E once; --auto-continue simulates Enter afterward.\n"
                         "--auto-crouch --auto-stand --auto-sprint test movement modes.\n"
                         "--auto-stop or --auto-release-sprint releases scripted input halfway.\n"
                         "--camera-view hides the normal HUD. --pitch DEGREES sets the camera tilt.\n"
                         "--audio-preview file.wav exports sounds without playback or a GL context.\n"
                         "--profile frames.csv records per-phase frame timings.\n"
                         "--unbounded-queue disables normal two-frame GPU pacing for comparisons.\n"
                         "--auto-walk: forward movement with fixed 1/60-second simulation "
                         "steps; use --demo to begin unpaused. FPS uses real elapsed time.\n"
                         "--ending-preview starts the elevator film; --ending-time N seeks to a test shot; F9 previews in-game.\n"
                         "--settings opens settings; --auto-settings verifies changing brightness.\n"
                         "--fps-limit N overrides the frame cap (0: unlimited; default: 60 in normal play).\n"
                         "--settings-file path uses a custom settings file (hidden runs otherwise ignore saved settings).\n"
                         "--auto-door / --auto-close-door verify a nearby door interaction.\n"
                         "--menu-check exercises settings and resume; --auto-pause pauses on frame 2.\n"
                         "--music-file path selects an optional local MP3/WAV/M4A song.\n"
                         "--door-close-frame N changes the scripted closing time (default 110).\n"
                         "Threat verification: --threat-x X --threat-z Z --threat-yaw DEGREES\n"
                         "--threat-preview freezes the model/AI. --no-threat isolates room checks.\n"
                         "--threat-time SECONDS --threat-speed MPS select an animation preview pose.\n"
                         "--auto-catch [--auto-restart | --auto-restart-click] checks the defeat flow.\n"
                         "--auto-scare-escape tests Escape during capture and game over.\n"
                         "--jumpscare-config PATH loads optional jumpscare timing/camera settings.\n"
                         "--balance-file PATH selects validated gameplay tuning (default balance.cfg).\n"
                         "--performance-report PATH writes bounded memory/resource/frame diagnostics JSON.\n"
                         "--slow-frame-ms N changes the stall threshold (default 50).\n"
                         "--run-seconds N ends after real wall-clock play time (0 disables).\n"
                         "--stress-cycles N repeats 0 -> Fun -> Parking -> 0; --stress-interval N sets frames per level.\n"
                         "Minimum window size: 960 x 640.\n";
            std::exit(0);
        } else {
            throw std::runtime_error("Unknown option: " + argument);
        }
    }

    if(!std::isfinite(options.slowFrameMs)||options.slowFrameMs<1||options.slowFrameMs>10000 ||
       !std::isfinite(options.runSeconds)||options.runSeconds<0||options.runSeconds>86400 ||
       options.stressCycles<0||options.stressCycles>1000||options.stressInterval<40||options.stressInterval>36000)
        throw std::runtime_error("Invalid performance threshold, duration or stress-test bounds.");
    if(options.stressCycles && !options.frames && options.runSeconds==0)
        options.frames=(options.stressCycles*3+1)*options.stressInterval;
    if (options.width < MIN_WINDOW_WIDTH || options.width > 7680 ||
        options.height < MIN_WINDOW_HEIGHT || options.height > 4320) {
        throw std::runtime_error("Window size must be between 960x640 and 7680x4320.");
    }
    if (options.frames < 0 || options.doorCloseFrame < 0 || !std::isfinite(options.spawnX) ||
        !std::isfinite(options.spawnZ) || !std::isfinite(options.yaw) || !std::isfinite(options.pitch) ||
        !std::isfinite(options.threatX) || !std::isfinite(options.threatZ) || !std::isfinite(options.threatYaw) ||
        !std::isfinite(options.threatPreviewTime) || options.threatPreviewTime<0 || options.threatPreviewTime>10000 ||
        !std::isfinite(options.threatPreviewHeadYaw) || std::abs(options.threatPreviewHeadYaw)>90 ||
        !std::isfinite(options.threatPreviewHeadPitch) || std::abs(options.threatPreviewHeadPitch)>30 ||
        !std::isfinite(options.threatPreviewSpeed) || options.threatPreviewSpeed<0 || options.threatPreviewSpeed>10 ||
        std::abs(options.threatX)>1e12 || std::abs(options.threatZ)>1e12 ||
        std::abs(options.pitch) > 85 ||
        std::abs(options.spawnX) > 1e12 || std::abs(options.spawnZ) > 1e12) {
        throw std::runtime_error("Invalid frame count or spawn coordinate.");
    }
    if (options.hidden && options.frames == 0 && options.runSeconds == 0) {
        options.frames = 60;
    }
    if(!options.hidden && !options.explicitSeed)
        options.seed=std::uint64_t(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    return options;
}

} // namespace br::app
