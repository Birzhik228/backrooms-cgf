#pragma once

#include "world.h"
#include <cstdint>
#include <string>

namespace br::app {

inline constexpr int MIN_WINDOW_WIDTH = 960;
inline constexpr int MIN_WINDOW_HEIGHT = 640;

struct Options {
    uint64_t seed = 12071998;
    int fpsLimit = -1; // -1: settings; hidden benchmarks remain uncapped.
    int frames = 0, width = 1280, height = 800;
    bool hidden = false, demo = false, tour = false;
    bool endingPreview=false,endingCheck=false;
    float endingTime=0;
    bool explicitSeed=false,explicitPitch=false,exitPreview=false,keyPreview=false;
    bool noFlashlight = false, noClues = false, map = false, metrics = false;
    bool wireframe = false, autoWalk = false, autoExit = false, autoContinue = false;
    bool autoCrouch = false, autoStand = false, autoSprint = false, cameraView = false;
    bool autoStop = false, autoReleaseSprint = false;
    bool unboundedQueue = false, settingsMenu = false, autoSettings = false, musicMenuCheck = false;
    bool autoDoor = false, autoCloseDoor = false, menuCheck = false, autoPause = false;
    bool partyPreview=false, partyCheck=false, funAmbiencePreview=false;
    bool noThreat=false, threatPreview=false, forcedThreat=false;
    bool funPreview=false, parkingPreview=false, autoBreakers=false, darknessPreview=false,parkingAmbiencePreview=false;
    bool autoVerity=false, autoKey=false, autoHide=false, hideFixture=false;
    bool mouseFixture=false, mousePreview=false, guideCheck=false, guideClose=false, guideClick=false;
    bool guideBack=false, guideSkip=false;
    bool levelMenuCheck=false,levelMenuClose=false,levelMenuClick=false,levelMenuResume=false,levelMenuBackClick=false;
    bool controlHoldCheck=false,controlPauseCheck=false,controlTapCheck=false;
    int observationPreview=-1;
    bool autoReadObservation=false,presencePreview=false;
    int levelMenuTarget=0,levelMenuAt=-1;
    bool autoCatch=false, autoRestart=false, autoRestartClick=false, autoScareEscape=false, autoJumpscare=false;
    double threatX=8, threatZ=3.75;
    float threatYaw=-90;
    float threatPreviewTime=0, threatPreviewSpeed=0, threatPreviewHeadYaw=0, threatPreviewHeadPitch=0;
    std::string settingsFile, musicFile;
    int doorCloseFrame=110;
    double spawnX = CELL*.5, spawnZ = CELL*.5;
    float yaw = -90, pitch = -3;
    std::string capture, audioPreview, profile;
    std::string performanceReport, balanceFile="balance.cfg";
    double slowFrameMs=50,runSeconds=0;
    int stressCycles=0,stressInterval=120;
    std::string jumpscareConfigFile="jumpscare.cfg";
};

Options parseOptions(int argc, char** argv);

} // namespace br::app
