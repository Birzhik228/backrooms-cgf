#include "game_internal.h"

namespace br::app {
namespace fs = std::filesystem;

void Game::restoreEndingAmbience(const char* reason) {
    if(muted||ending.finished())return;
    const float time=ending.time();
    SoundKind kind;float gain,start;const char* bed;
    if(time>=23) { kind=SoundKind::CityAmbience;gain=.70f;start=23;bed="city"; }
    else if(time>=13) { kind=SoundKind::SurfaceAmbience;gain=.60f;start=13;bed="lobby"; }
    else if(time>=3&&time<10) { kind=SoundKind::ElevatorMotor;gain=.70f;start=3;bed="motor"; }
    else return;
    if(endingCueTime<start)return; // The ordinary cue already fired at this frame's boundary.
    // Seeking/unmuting restores a bed, never a past door or arrival bell.
    // The caller has already installed the cinematic scene.
    audio.play(kind,{0,1.65,-1},gain);
    log(std::string("Ending ambience restored: ")+bed+" ("+reason+")");
}

void Game::startEnding(const char* reason,float at) {
    ending.start(at);endingPaused=false;endingFinishedLogged=false;
    endingPhase.clear();endingCueTime=at>0?at:-1;
    endingAmbienceRestore=at>0?"seek":nullptr;
    endingFrozenX=player.x;endingFrozenZ=player.z;endingFrozenStamina=player.stamina.value;
    endingSelection=1;player.motion.reset();setScreen(Screen::Ending);
    flow.guidePending=false;notice.clear();
    audio.reset(static_cast<std::uint32_t>(world.seed()));
    AudioScene silent;silent.cinematic=true;silent.lightPower=0;audio.setScene(silent);
    music.update(true,true,0,0);
    log(std::string("Ending started: ")+reason);
}

bool Game::renderEnding() {
    if(flow.cinematic()) {
        int width,height;glfwGetFramebufferSize(window,&width,&height);
        const bool focused=options.hidden||glfwGetWindowAttrib(window,GLFW_FOCUSED);
        if(!focused&&!endingPaused&&!ending.finished()) {
            endingPaused=true;log("Ending paused.");
        }
        if(input.hit(GLFW_KEY_ESCAPE)&&!ending.finished()) {
            endingPaused=!endingPaused;log(endingPaused?"Ending paused.":"Ending resumed.");
        }
        if(input.hit(GLFW_KEY_M)) {
            muted=!muted;
            if(muted) { audio.stop();endingAmbienceRestore=nullptr; }
            else { audio.start();endingAmbienceRestore="unmute"; }
            log(muted?"Ending sound: off":"Ending sound: on");
        }
        if(input.hit(GLFW_KEY_Q)&&(endingPaused||ending.finished()))glfwSetWindowShouldClose(window,GL_TRUE);
        if(input.hit(GLFW_KEY_SPACE)&&!ending.finished()) {
            ending.start(EndingSequence::DURATION);endingPaused=false;audio.reset(static_cast<std::uint32_t>(world.seed()));
            endingAmbienceRestore=nullptr;
            log("Ending skipped.");
        }
        if(ending.finished()) {
            if(input.hit(GLFW_KEY_UP)||input.hit(GLFW_KEY_DOWN))endingSelection=1-endingSelection;
            int action=input.hit(GLFW_KEY_R)?0:input.hit(GLFW_KEY_ENTER)?endingSelection:-1;
            if(input.clicked) {
                int ww,wh;glfwGetWindowSize(window,&ww,&wh);double mx,my;glfwGetCursorPos(window,&mx,&my);
                action=EndingButtons(width,height).at(mx*width/std::max(1,ww),my*height/std::max(1,wh));
            }
            if(action==0) { ++endingReplays;startEnding("replay");log("Ending replayed."); }
            if(action==1) {
                endingPaused=false;endingRenderer.destroy();
                endingAmbienceRestore=nullptr;
                audio.reset(static_cast<std::uint32_t>(world.seed()));audio.setScene(AudioScene{});
                audio.setPaused(true);audio.setVolume(settings.volume);music.update(true,muted||!settings.musicEnabled,settings.volume*settings.musicVolume,0);
                setScreen(Screen::Menu);input.pressed.fill(false);input.clicked=false;menuSelection=0;
                notice.clear();noticeTime=0;log("Ending returned to menu.");
            }
        }
        if(flow.cinematic()) {
            const bool suspended=endingPaused||!focused||width<=0||height<=0;
            const float endingDelta=options.hidden?dt:float(std::min(timing.rawDelta,.25));
            const auto previousPose=sampleEndingPose(ending.time());
            ending.update(endingDelta,suspended);
            endingWorldFrozen=endingWorldFrozen&&player.x==endingFrozenX&&player.z==endingFrozenZ&&player.stamina.value==endingFrozenStamina;
            const std::string phase=EndingSequence::phaseName(ending.time());
            if(phase!=endingPhase){
                endingPhase=phase;log("Ending phase: "+phase);
                if(phase=="City life")log("Ending city: "+std::to_string(EndingCity::pedestrianCount)+
                    " walking pedestrians / "+std::to_string(EndingCity::carCount)+" moving cars.");
            }
            if(ending.finished()&&!endingFinishedLogged) {
                ++endingCompletions;endingFinishedLogged=true;audio.reset(static_cast<std::uint32_t>(world.seed()));
                endingAmbienceRestore=nullptr;
                log("Ending finished: You found the way out.");
            }
            AudioScene cinematic;cinematic.cinematic=true;cinematic.lightPower=0;
            cinematic.listener={0,1.65,0};cinematic.forward={0,0,-1};audio.setScene(cinematic);
            audio.setVolume(settings.volume*(1-endingEase(39,EndingSequence::DURATION,ending.time())));audio.setPaused(suspended||ending.finished());
            auto cue=[&](float at,SoundKind kind,float gain) {
                if(!suspended&&endingCueTime<at&&ending.time()>=at&&!muted&&!ending.finished())audio.play(kind,{0,1.65,-1},gain);
            };
            if(!suspended&&endingCueTime<10&&ending.time()>=10&&!ending.finished())
                audio.reset(static_cast<std::uint32_t>(world.seed())); // End even a recently restored motor before the arrival cues.
            if(!suspended&&endingCueTime<23&&ending.time()>=23&&!ending.finished())
                audio.reset(static_cast<std::uint32_t>(world.seed())); // Replace the muffled lobby bed with full street ambience outside.
            cue(0,SoundKind::ElevatorDoor,.7f);cue(3,SoundKind::ElevatorMotor,.7f);
            cue(10,SoundKind::ElevatorBell,.55f);cue(10,SoundKind::ElevatorDoor,.65f);
            cue(13,SoundKind::SurfaceAmbience,.6f);cue(20,SoundKind::Door,.5f);
            cue(23,SoundKind::CityAmbience,.70f);
            if(endingAmbienceRestore&&!suspended) {
                restoreEndingAmbience(endingAmbienceRestore);endingAmbienceRestore=nullptr;
            }
            if(!suspended)endingCueTime=ending.time();
            const auto currentPose=sampleEndingPose(ending.time());
            const float cinematicTravel=std::hypot(currentPose.eye.x-previousPose.eye.x,currentPose.eye.z-previousPose.eye.z);
            audio.update(endingDelta,cinematicTravel,false,false,Surface::HardFloor,suspended||ending.finished());
            music.update(true,true,0,dt);
            if(width<=0||height<=0){glfwWaitEventsTimeout(.03);return true;}
            endingRenderer.draw(ending,width,height,settings.brightness);
            drawEndingUI(ui,width,height,ending,suspended,endingSelection);ui.render(width,height);
            const bool lastFrame=options.frames>0&&frame+1>=options.frames;
            if(lastFrame&&!options.capture.empty())screenshot(options.capture,width,height);
            if(input.hit(GLFW_KEY_F12))screenshot(fs::path("captures")/"ending.bmp",width,height);
            const auto error=glGetError();if(error!=GL_NO_ERROR)throw std::runtime_error("Ending OpenGL error "+std::to_string(error));
            if(!options.unboundedQueue)frameQueue.submit();
            glfwSwapBuffers(window);
            int cap=options.fpsLimit>=0?options.fpsLimit:options.hidden?0:settings.maxFps;
            if(!options.hidden&&(suspended||ending.finished()))cap=cap>0?std::min(cap,30):30;
            const double workMs=(glfwGetTime()-timing.frameStart)*1000;
            if(cap>0)framePacer.wait(1.0/cap-(glfwGetTime()-timing.frameStart));
            ++frame;const double seconds=glfwGetTime()-timing.frameStart;
            maxFrameMs=std::max(maxFrameMs,seconds*1000);
            performance.record(frame,seconds*1000,workMs,cap>0?1000.0/cap:0,
                int(world.level()),true,world.chunks().size(),world.preparedChunks().size(),gpu.size());
            fpsTime+=seconds;++fpsFrames;if(fpsTime>=.5){fps=float(fpsFrames/fpsTime);fpsTime=0;fpsFrames=0;}
            if(lastFrame)glfwSetWindowShouldClose(window,GL_TRUE);
            return true; // Freeze streaming, AI, doors and ordinary game audio during the film.
        }
    }
    return false;
}

} // namespace br::app
