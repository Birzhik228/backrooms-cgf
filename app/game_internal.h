#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <algorithm>
#include <array>
#include <chrono>
#include <thread>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "../world.h"
#include "../common/math.h"
#include "../common/render_limits.h"
#include "../common/Shader.h"
#include "../common/graphics.h"
#include "../common/textures.h"
#include "../common/ui.h"
#include "../common/controls_guide.h"
#include "../common/level_menu.h"
#include "../common/ending_renderer.h"
#include "../common/ending_ui.h"
#include "../common/audio.h"
#include "../common/atmosphere.h"
#include "../common/presence.h"
#include "../common/observations.h"
#include "../common/fun_clues.h"
#include "../common/fun_audio.h"
#include "../common/parking_objective.h"
#include "../common/parking_audio.h"
#include "../common/movement.h"
#include "../common/gameplay.h"
#include "../common/player_state.h"
#include "../common/settings.h"
#include "../common/music.h"
#include "../common/threat.h"
#include "../common/mouse.h"
#include "../common/partygoer_model.h"
#include "../common/darkness.h"
#include "../common/threat_model.h"
#include "../common/encounter.h"

#include "options.h"
#include "input.h"
#include "diagnostics.h"
#include "render_resources.h"
#include "minimap.h"
#include "flow.h"
#include "performance.h"
#include "game_state.h"

namespace br::app {
// Internal application coordinator. Public entry point is game.h.
class Game {
public:
    Game(const Options& options, GLFWwindow* window) : options(options), window(window) {}
    int execute();
private:
    const Options& options;
    GLFWwindow* window;
    void initialize();
    void setScreen(Screen screen);
    void resetPlayer();
    void replaceWorld(uint64_t seed, Level level);
    void freshRun(uint64_t seed);
    void enterLevel(Level next,bool selected=false);
    void restoreEndingAmbience(const char* reason);
    void startEnding(const char* reason,float at=0);
    void simulateKey(int key);
    void applyScenarioInput();
    void handleMenus();
    void saveSettings();
    bool updateGameplay();
    void updateAudio();
    bool renderWorld();
    void drawHud();
    bool finishFrame();
    bool renderEnding();
    void reportAndShutdown();
    Vec3 headCenter(const std::vector<Vertex>& vertices);

    // Gameplay phases: kept in frame order by updateGameplay().
    struct InteractionResult { bool pickedUpKey; float doorNoise; };
    void updatePlayerMovement();
    void updateWorldStreaming(const std::optional<WorldPoint>& threatPosition);
    bool updateLevelExit();
    InteractionResult updateInteractions();
    bool collectLevelKey();
    bool interactWithBreaker();
    bool interactWithPresent();
    bool collectVerity(bool otherInteraction);
    bool interactWithHidingPlace(bool otherInteraction);
    void updateNearbyDoor();
    bool readObservation();
    float interactWithDoor(bool otherInteraction);
    void updateCreatures(const InteractionResult& interactions);
    void updateMouse(const ThreatPlayer& perceivedPlayer);
    void updatePartygoers();
    void updateDarkness();
    void updateAranea(const ThreatPlayer& perceivedPlayer,ThreatState previousThreatState);
    void updateEncounter();
    void updateThreatAnimation();

    // Verification hooks preserve their original phase-local ordering.
    bool usesFixedTimeStep() const;
    void applyMovementScenario();
    bool scenarioCrouchHeld() const;
    bool scenarioSprintHeld() const;
    float forwardMovementInput() const;
    void applyInteractionScenario();
    void applyVerityScenario();
    bool doorInteractionRequested() const;
    void applyAraneaContactScenario();
    void applyMouseContactScenario(ThreatPlayer& perceivedPlayer);
    void applyMousePreviewScenario();
    void applyPartygoerScenario();
    void applyDarknessScenario();

    // World rendering: local RenderFrame values flow through ordered passes.
    struct RenderFrame;
    void uploadWorldMeshes();
    void updateThreatMesh();
    bool beginWorldTarget();
    void updateLocalMeshes(const RenderFrame& render);
    void updateRenderCamera(RenderFrame& render);
    void collectRenderLights(RenderFrame& render);
    void drawShadowGeometry(const RenderFrame& render, const Mat4& lightVP, bool actor);
    void renderShadows(const RenderFrame& render);
    void configureWorldShader(const RenderFrame& render);
    void drawRoomGeometry(const RenderFrame& render);
    void drawWorldObjects(const RenderFrame& render);
    void drawThreatModel(const RenderFrame& render);
    void renderLens(const Mat4& projection);

    // This member is destroyed last, after every GL resource below it.
    PerformanceMonitor performance{options.performanceReport,options.slowFrameMs};
    int stressTransitions=0,lastStressTransitionFrame=-1,lastStressSampleFrame=-1;

    // Persistent session, renderer and player state.
    Settings settings;
    int mouseSqueaks=0;
    float presentMouseTime=0;
    const std::string settingsPath = options.settingsFile.empty() ? "settings.cfg" : options.settingsFile;
    Shader scene{"vshader_backrooms.glsl", "fshader_backrooms.glsl"};
    Shader lens{"vshader_lens.glsl", "fshader_lens.glsl"};
    UI ui;
    Textures textures = createTextures();
    SceneTarget target;
    ShadowMaps shadows;
    AmbientTarget ambientTarget;
    Shader shadow{"vshader_shadow.glsl","fshader_shadow.glsl"};
    Shader ao{"vshader_lens.glsl","fshader_ao.glsl"};
    std::uint64_t lampShadowUpdates=0;
    gl::VertexArray emptyVao;
    World world{options.seed,options.parkingPreview?Level::Parking:options.funPreview?Level::Fun:Level::Zero};
    PlayerState player{options.spawnX, options.spawnZ};
    Threat threat{world.seed()};
    Mouse mouse;
    JumpscareManager encounter{loadJumpscareConfig(options.jumpscareConfigFile)};
    const JumpscareConfig& scareConfig=encounter.config();

    ThreatVocalCadence threatVocals;
    Level0Atmosphere atmosphere{world.seed()};
    Level0Presence presence{world.seed()};
    FunAudioDirector funAudio{world.seed()};
    ParkingAudioDirector parkingAudio{world.seed()};
    int parkingPipeEvents=0,parkingDripEvents=0,parkingVentEvents=0,parkingRelayEvents=0;
    int partyStepEvents=0,partyFrozenSteps=0,funBalloonEvents=0,funMelodyEvents=0;
    float veritySongTime=0,partyStepDuckTime=0;
    int atmosphereAmbient=0,atmosphereSteps=0,atmosphereHints=0;
    int presenceEvents=0,observationsRead=0,observationKind=-1;
    float observationTime=0;

    float occlusionClock=0,threatTransmission=1;
    float scareYaw=0, scarePitch=0, scareTargetYaw=0, scareTargetPitch=0;
    Vec3 scareApproach{0,0,-1};
    Input input;
    std::map<ChunkCoord, GpuChunk> gpu;
    GpuChunk doorGpu, threatGpu, keyGpu, mouseGpu, partyGpu, verityGpu, darknessGpu;
    Darkness darkness;
    Partygoers partygoers;
    std::array<double,Partygoers::COUNT*5> lastPartyPose{};
    Vec3 threatHeadLocal{0,THREAT_EYE_HEIGHT,0};
    float threatAnimation=0, lastThreatMeshTime=-1;
    float threatStoop=0, lastThreatStoop=-1;
    float threatGaitPhase=0, threatVisualSpeed=0, threatHeadYaw=0, threatHeadPitch=0;
    float threatTurnWeight=0;
    ThreatFootPlanting threatFootPlanting;
    float threatBodyYaw=threat.yaw();
    ThreatFootContacts lastThreatContact{};
    bool threatRunningSteps=false;
    int threatFootfalls=0,threatWalkFootfalls=0,threatRunFootfalls=0;
    int threatNotices=0,threatRoars=0;
    bool lastScareMesh=false, scriptedRestarted=false;
    int catches=0, restarts=0;
    bool doorVisualDirty=true;
    ChunkCoord doorOrigin{};
    std::size_t doorShift=0;
    BackgroundMusic music;
    const bool musicLoaded = music.load(options.musicFile.empty() ? std::filesystem::path("assets/audio/music/numbers-temporex.mp3") : std::filesystem::path(options.musicFile));
    bool musicErrorReported=!music.error().empty();
    Audio audio;
    bool muted = options.hidden, flashlight = !options.noFlashlight, clues = !options.noClues;
    GameFlow flow;
    bool showMap = options.map, diagnostics = options.metrics;
    int guidePage=0;
    bool wireframe = options.wireframe, shifting = true;
    bool exitUsed = false, exitArmed = true;
    EndingSequence ending;
    EndingRenderer endingRenderer;
    bool endingPaused=false,endingFinishedLogged=false;
    int endingCompletions=0,endingReplays=0,endingSelection=1;
    std::string endingPhase;
    float endingCueTime=-1;
    const char* endingAmbienceRestore=nullptr;
    double endingFrozenX=player.x,endingFrozenZ=player.z;
    float endingFrozenStamina=100;
    bool endingWorldFrozen=true;

    int levelMenuSelection=0,levelMenuVisits=0,levelMenuTravel=0;
    int scriptedSelectorOpened=-1;
    int menuSelection = 0, settingSelection = 0, doorsUsed = 0;
    float tension = 0;
    bool cameraView = options.cameraView;
    float fieldOfView = settings.fov;
    double runStart = 0;
    double lastTime = 0, elapsed = 0;
    double shiftClock = 0, fpsTime = 0, maxFrameMs = 0;
    int frame = 0, fpsFrames = 0;
    float fps = 0, noticeTime = 0;
    size_t maxChunks = 0, maxPrepared = 0;
    std::string notice;
    FrameQueue frameQueue;
    FramePacer framePacer;
    std::ofstream profile;

    // Scratch values shared by the ordered phases of one frame.
    FrameTimings timing;
    float dt=0;
    std::optional<std::array<double,2>> scriptedMenuClick;
    bool scareSuspended=false,moving=false,sprinting=false;
    Vec3 forward{};
    double frameDistance=0,oldThreatX=0,oldThreatZ=0;
    bool nearDoor=false,nearPresent=false,nearHide=false,nearBreaker=false,nearObservation=false,nearVerity=false;
    DoorInfo nearbyDoor{};
    Present nearbyPresent{};
    int nearbyBreaker=-1;
    RoomInfo currentRoom{};
    unsigned uploads=0;
    size_t residentTriangles=0,drawnTriangles=0,lightCount=0;
    int width=0,height=0,drawn=0;
    ChunkCoord origin{};
    Vec3 cameraForward{};
    float shakeRoll=0,renderFov=0;
    JumpscareShake scareShake{};
};
} // namespace br::app
