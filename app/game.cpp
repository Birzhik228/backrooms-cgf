#include "game_internal.h"
#include "gl_context.h"

namespace br::app {
namespace fs = std::filesystem;

void Game::initialize() {
    if (!options.hidden || !options.settingsFile.empty()) settings.load(settingsPath);
    glfwSwapInterval(options.hidden ? 0 : int(settings.vsync));
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_FRAMEBUFFER_SRGB);
    glClearColor(0, 0, 0, 1); // Matches the fully faded distance fog.

    shadows.init();
    emptyVao.create();

    scene.use();
    scene.set("uWallpaper", 0);
    scene.set("uCarpet", 1);
    scene.set("uCeiling", 2);
    scene.set("uWallpaperNormal", 3);
    scene.set("uCarpetNormal", 4);
    scene.set("uCeilingNormal", 5);
    scene.set("uHardFloor", 6);
    scene.set("uHardFloorNormal", 7);
    scene.set("uCreatureColor",10);scene.set("uCreatureNormal",11);
    lens.use();
    lens.set("uScene", 0);
    lens.set("uAO",1);lens.set("uDepth",2);
    scene.use();scene.set("uFlashShadow",8);scene.set("uLampShadows",9);
    ao.use();ao.set("uDepth",0);

    if(options.exitPreview){auto p=world.exitLocation();px=p.x;pz=p.z+.8;}
    if(options.keyPreview){auto p=world.keyLocation();px=p.x;pz=p.z+2.5;}
    world.update(px, pz);
    if (world.blocked(px, pz, .23)) {
        throw std::runtime_error("Requested spawn intersects a wall.");
    }

    mouse.reset(world.seed());
    log("Jumpscare: duration "+std::to_string(scareConfig.duration)+" s, turn "+std::to_string(scareConfig.cameraTurnDuration)+" s, face "+std::to_string(scareConfig.faceDistance)+" m.");
    if(options.forcedThreat && !options.noThreat && world.level()==Level::Zero) {
        if(world.blocked(options.threatX,options.threatZ,Threat::RADIUS))
            throw std::runtime_error("Threat fixture intersects a wall or unloaded chunk.");
        threat.resetAt(options.threatX,options.threatZ,options.threatYaw*PI/180);
    }
    input.window = window;
    input.setLook(options.yaw, options.pitch);
    if(options.observationPreview>=0 && world.level()==Level::Zero) {
        std::vector<Level0Observation> notes;
        for(const auto& entry:world.chunks())
            notes.insert(notes.end(),entry.second.observations.begin(),entry.second.observations.end());
        std::sort(notes.begin(),notes.end(),[](const auto& a,const auto& b){return a.id<b.id;});
        if(notes.empty())throw std::runtime_error("No Level 0 observation available for preview.");
        const auto& note=notes[std::min(std::size_t(options.observationPreview),notes.size()-1)];
        const double x=note.position.x-1.0,z=note.position.z-1.0;
        if(world.blocked(x,z,.23))throw std::runtime_error("Observation preview approach is blocked.");
        px=x;pz=z;world.update(px,pz);
        input.setLook(45,options.explicitPitch?options.pitch:-46);
        log("Observation preview: kind "+std::to_string(note.kind)+" / "+std::to_string(note.position.x)+", "+std::to_string(note.position.z));
    }
    threatBodyYaw = threat.yaw();
    input.sensitivity = settings.sensitivity;
    glfwSetWindowUserPointer(window, &input);
    glfwSetCursorPosCallback(window, Input::mouse);
    glfwSetKeyCallback(window, Input::key);
    glfwSetMouseButtonCallback(window, Input::button);
    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }
    flow.guidePending=(!options.hidden && !options.tour) || options.guideCheck;
    setScreen(options.settingsMenu ? Screen::Settings :
              (options.demo || options.tour) ? Screen::Gameplay : Screen::Menu);
    flow.entered = !input.paused;

    // Aim the capture at the visible skull, which projects ahead of the root.
    // Read and validate the imported animation cache during loading, so the
    // first encounter never has to open a file or allocate the clip bank.
    {
        const auto initialModel=threatModelVertices(0,0,false);
        uploadVertices(threatGpu,initialModel,GL_DYNAMIC_DRAW);
        threatHeadLocal=headCenter(initialModel);
        log("Loaded Aranea model: "+std::to_string(initialModel.size()/3)+" triangles.");
    }
    if(musicLoaded) log("Background music loaded.");
    else if(!music.error().empty()) log("Background music unavailable: "+music.error());
    else log("No optional music file found; environmental sound remains available.");
    audio.setVolume(settings.volume);
    audio.reset(static_cast<uint32_t>(options.seed));

    if (!muted) {
        audio.start();
        log(audio.available() ? "Audio output active." : "Audio output unavailable; use M to retry.");
    }


    fieldOfView = settings.fov;
    runStart = lastTime = glfwGetTime();
    performance.mark("session-start",int(world.level()),world.chunks().size(),world.preparedChunks().size(),gpu.size());
    if(options.endingPreview)startEnding("preview",options.endingTime);

    if (!options.profile.empty()) {
        profile.open(options.profile);
        if (!profile) throw std::runtime_error("Cannot open frame profile: " + options.profile);
        profile << "frame,x,z,chunk_x,chunk_z,world_ms,upload_ms,frame_ms,uploads,lights,scene_ms,lens_ms,ui_ms,swap_ms,input_ms,move_ms,audio_ms,setup_ms,queue_wait_ms,resident_triangles,drawn_triangles,scare_elapsed,turn_weight,face_distance,fov,shake_x,shake_y,shake_z,shake_roll,look_x,look_y,look_z,threat_x,threat_z,body_yaw,head_yaw,forward_dot\n";
    }
}

int run(const Options& options) {
    glfwSetErrorCallback(glfwError);
    WindowContext context(options.width,options.height,
        "BACKROOMS / Threshold - CGF Prototype v0.12",options.hidden);
    glfwSetWindowSizeLimits(context.window(),MIN_WINDOW_WIDTH,MIN_WINDOW_HEIGHT,
                           GLFW_DONT_CARE,GLFW_DONT_CARE);
    log(std::string("OpenGL: ")+reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    log(std::string("Renderer: ")+reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    struct LifetimeReport {
        ~LifetimeReport() noexcept {
            // Must not replace an original loading/rendering failure.
            try { log("GL resources after Game: "+std::to_string(gl::snapshot().totalAlive())); }
            catch(...) {}
        }
    } lifetimeReport;
    // Reverse destruction order: Game -> lifetime report -> window/context.
    // This also handles an exception in a Game member's constructor.
    Game game(options,context.window());
    return game.execute();
}

} // namespace br::app
