#include "game_internal.h"
#include "common/presentation.h"

namespace br::app {
namespace fs = std::filesystem;

void Game::simulateKey(int key) {
    Input::key(window,key,0,GLFW_PRESS,0);
}

void Game::applyScenarioInput() {
    scriptedMenuClick.reset();
    applyPresentationScenario();
    applyDemoControlsScenario();
    // Bounded, opt-in integration fixture: exercise the real level lifecycle.
    if(options.stressCycles && frame>0 && frame%options.stressInterval==0 &&
       stressTransitions<options.stressCycles*3 && lastStressTransitionFrame!=frame) {
        enterLevel(Level((int(world.level())+1)%3),true);
        ++stressTransitions;lastStressTransitionFrame=frame;
    }
    if(options.stressCycles && frame%options.stressInterval==options.stressInterval-1 &&
       lastStressSampleFrame!=frame) {
        performance.mark("level-settled",int(world.level()),world.chunks().size(),world.preparedChunks().size(),gpu.size());
        lastStressSampleFrame=frame;
    }
    const int selectorStart=options.levelMenuAt>=0?options.levelMenuAt:(options.autoKey||options.autoBreakers)?12:2;
    if(options.levelMenuCheck && frame>=selectorStart && levelMenuVisits==0 && !encounter.scaring()) {
        if(flow.is(Screen::Controls) || !input.paused)simulateKey(GLFW_KEY_ESCAPE);
        else if(options.levelMenuClick) {
            int w,h;glfwGetFramebufferSize(window,&w,&h);
            scriptedMenuClick=std::array<double,2>{w*.5,(h-550)*.5+163+2*48+20};
            Input::button(window,GLFW_MOUSE_BUTTON_LEFT,GLFW_PRESS,0);
        } else if(menuSelection!=2)simulateKey(GLFW_KEY_DOWN);
        else simulateKey(GLFW_KEY_ENTER);
    }
    if(options.controlHoldCheck && (frame==2||frame==32))
        input.onKey(GLFW_KEY_LEFT_CONTROL,frame==2?GLFW_PRESS:GLFW_RELEASE);
    if(options.controlPauseCheck && (frame==2||frame==4))
        input.onKey(GLFW_KEY_LEFT_CONTROL,frame==2?GLFW_PRESS:GLFW_RELEASE);
    if(options.controlTapCheck && (frame==selectorStart||frame==selectorStart+1))
        input.onKey(GLFW_KEY_LEFT_CONTROL,frame==selectorStart?GLFW_PRESS:GLFW_RELEASE);
    if(options.controlPauseCheck&&frame==3)simulateKey(GLFW_KEY_ESCAPE);
    if(options.autoReadObservation && frame==3)simulateKey(GLFW_KEY_E);
    if(options.levelMenuResume && scriptedSelectorOpened>=0 && frame==scriptedSelectorOpened+2)
        simulateKey(GLFW_KEY_ESCAPE);
    if(options.levelMenuCheck && flow.is(Screen::LevelSelect)) {
        if(scriptedSelectorOpened<0)scriptedSelectorOpened=frame;
        if((options.levelMenuClose||options.levelMenuResume) && frame==scriptedSelectorOpened+1)
            simulateKey(GLFW_KEY_ESCAPE);
        else if(options.levelMenuBackClick && frame==scriptedSelectorOpened+1) {
            int w,h;glfwGetFramebufferSize(window,&w,&h);const LevelMenuLayout box(w,h);
            scriptedMenuClick=std::array<double,2>{box.left+360*box.scale,box.top+455*box.scale};
            Input::button(window,GLFW_MOUSE_BUTTON_LEFT,GLFW_PRESS,0);
        } else if(options.levelMenuTarget && frame>=scriptedSelectorOpened+1 && frame<=scriptedSelectorOpened+6) {
            if(options.levelMenuClick && frame==scriptedSelectorOpened+1) {
                int w,h;glfwGetFramebufferSize(window,&w,&h);const LevelMenuLayout box(w,h);
                scriptedMenuClick=std::array<double,2>{box.left+360*box.scale,
                    box.top+(180+(options.levelMenuTarget-1)*86)*box.scale};
                Input::button(window,GLFW_MOUSE_BUTTON_LEFT,GLFW_PRESS,0);
            } else if(!options.levelMenuClick) {
                if(levelMenuSelection!=options.levelMenuTarget-1)simulateKey(GLFW_KEY_DOWN);
                else simulateKey(GLFW_KEY_ENTER);
            }
        }
    }
    if(options.autoSettings && frame<=2) simulateKey(frame==2 ? GLFW_KEY_LEFT : GLFW_KEY_DOWN);
    if(options.musicMenuCheck && frame<=7) simulateKey(frame==7 ? GLFW_KEY_ENTER : GLFW_KEY_DOWN);
    if(options.menuCheck && frame<=6) {
        const int keys[]={GLFW_KEY_DOWN,GLFW_KEY_ENTER,GLFW_KEY_DOWN,GLFW_KEY_RIGHT,GLFW_KEY_ESCAPE,GLFW_KEY_UP,GLFW_KEY_ENTER};
        if(!options.autoExit || frame<5) simulateKey(keys[frame]);
        else if(frame==5) simulateKey(GLFW_KEY_ESCAPE);
    }
    if(options.menuCheck && options.autoExit && frame==10) simulateKey(GLFW_KEY_ENTER);
    if(options.autoPause && frame==2) simulateKey(GLFW_KEY_ESCAPE);
    if(options.autoScareEscape && (frame==10 || frame==75)) simulateKey(GLFW_KEY_ESCAPE);
    if(options.autoJumpscare && (frame==2 || (options.autoRestart && frame==170))) simulateKey(GLFW_KEY_J);
    if((options.autoRestart || options.autoRestartClick) && encounter.gameOver() && frame>=90 && !scriptedRestarted) {
        if(options.autoRestartClick) {
            // Hidden windows cannot warp the OS pointer. Supply the same
            // framebuffer point consumed by the real menu hit test instead.
            int w,h; glfwGetFramebufferSize(window,&w,&h);
            scriptedMenuClick=std::array<double,2>{w*.5,(h-550)*.5+184};
            Input::button(window,GLFW_MOUSE_BUTTON_LEFT,GLFW_PRESS,0);
        } else simulateKey(GLFW_KEY_ENTER);
        scriptedRestarted=true;
    }
    if(options.autoHide && (frame==5 || frame==95)) simulateKey(GLFW_KEY_E);
    if(options.guideCheck && frame==1 && !flow.entered) simulateKey(GLFW_KEY_ENTER);
    if(options.guideCheck && (frame==5 || frame==7 || frame==9) && options.guideClose) simulateKey(GLFW_KEY_ENTER);
    if(options.guideCheck && (frame==5 || frame==7 || frame==9) && options.guideClick) Input::button(window,GLFW_MOUSE_BUTTON_LEFT,GLFW_PRESS,0);
    if(options.guideCheck && frame==8 && options.guideBack) simulateKey(GLFW_KEY_LEFT);
    if(options.guideCheck && frame==5 && options.guideSkip) simulateKey(GLFW_KEY_ESCAPE);
    if(options.endingCheck) {
        // Hidden verification begins muted; enable it before testing off/on.
        if((options.hidden&&frame==1)||frame==18||frame==20)simulateKey(GLFW_KEY_M);
        if(frame==5||frame==15)simulateKey(GLFW_KEY_ESCAPE);
        if(frame==25||frame==32)simulateKey(GLFW_KEY_SPACE);
        if(frame==27)simulateKey(GLFW_KEY_R);
        if(frame==34)simulateKey(GLFW_KEY_ENTER);
    }
    input.poll();
}

// Gameplay fixtures run at the phase they observe or prepare. Moving all of
// them to frame start would change collision, perception and interaction order.
void Game::applyMovementScenario() {
    if(options.partyCheck && frame==60)input.setLook(90,0);
    if(options.partyCheck && frame==120)input.setLook(-90,0);
}

bool Game::scenarioCrouchHeld() const {
    return (options.controlHoldCheck && frame>=2 && frame<32) ||
        (options.autoCrouch && !(options.autoStand && frame >= options.frames / 2));
}

bool Game::scenarioSprintHeld() const {
    return options.autoSprint && !(options.autoReleaseSprint && frame >= options.frames / 2);
}

float Game::forwardMovementInput() const {
    return options.autoWalk ? ((options.autoStop && frame >= options.frames / 2) ? 0.f : 1.f) :
        (input.down(GLFW_KEY_W) ? 1.f : 0.f) - (input.down(GLFW_KEY_S) ? 1.f : 0.f);
}

void Game::applyInteractionScenario() {
    if(options.hideFixture && frame==1) {
        const br::HidingSpot* selected=nullptr;
        for(const auto& entry:world.chunks()) for(const auto& spot:entry.second.hidingSpots)
            if(!selected || spot.id<selected->id) selected=&spot;
        if(selected) {
            player.x=selected->approach.x;player.z=selected->approach.z;
            input.setLook(float(std::atan2(selected->inside.z-player.z,selected->inside.x-player.x)*180/3.141592653589793),-12);
            log("Hiding fixture: "+std::to_string(player.x)+", "+std::to_string(player.z));
        }
    }
    // Scripted fixtures use the same E interaction and present rolls as play.
    if(options.autoBreakers && world.isParking() && frame>=1 && frame<=9 && frame%4==1) {
        const auto panel=world.breakerLocation((frame-1)/4);
        player.x=panel.x;player.z=panel.z+1;world.update(player.x,player.z);player.motion.reset();input.setLook(-90,0);forward=input.forward();
        simulateKey(GLFW_KEY_E);
    }
    if(options.autoBreakers && options.autoExit && world.isParking() && frame==20) {
        const auto exit=world.exitLocation();player.x=exit.x;player.z=exit.z;world.update(player.x,player.z);player.motion.reset();input.setLook(-90,0);forward=input.forward();
    }
    if(options.autoBreakers && options.autoExit && world.isParking() && frame==21)simulateKey(GLFW_KEY_E);
    if(options.funPreview && options.autoKey && world.isFun() && !world.keyCollected() &&
       !(options.autoVerity && (!world.verityDrops().empty() || world.infiniteStamina())) &&
       frame>0 && frame<65 && frame%4==1) {
        double nearest=1e30;Present selected{};
        for(const auto& entry:world.chunks())for(const auto& gift:entry.second.presents)if(!gift.opened) {
            const double distance=std::hypot(gift.x-player.x,gift.z-player.z);
            if(distance<nearest){nearest=distance;selected=gift;}
        }
        if(nearest<1e30){player.x=selected.x+1.05;player.z=selected.z;input.setLook(180,-35);}
        world.update(player.x,player.z);forward=input.forward();
        simulateKey(GLFW_KEY_E);
    }
    if(options.funPreview && options.autoKey && options.autoExit && world.keyCollected() && frame==80) {
        const auto exit=world.exitLocation();player.x=exit.x;player.z=exit.z;world.update(player.x,player.z);player.motion.reset();input.setLook(-90,0);
        forward=input.forward();
    }
    if(options.autoKey && frame==1 && world.level()==Level::Zero) {
        const auto key=world.keyLocation(); player.x=key.x;player.z=key.z+1;world.update(player.x,player.z);
    }
    if(options.autoKey && options.autoExit && frame==20 && world.level()==Level::Zero) {
        const auto exit=world.exitLocation();player.x=exit.x;player.z=exit.z;world.update(player.x,player.z);
    }
}

void Game::applyVerityScenario() {
    if(options.autoVerity && frame==3 && !world.verityDrops().empty()) {
        const auto drop=world.verityDrops().front();player.x=drop.x+.5;player.z=drop.z;player.motion.reset();
        input.setLook(180,-71);forward=input.forward();
    }
    if(options.autoVerity && frame==5)simulateKey(GLFW_KEY_E);
}

bool Game::doorInteractionRequested() const {
    return input.hit(GLFW_KEY_E) ||
        (options.autoExit && frame == (options.autoKey ? (options.funPreview?81:21) : options.menuCheck ? 8 : 2)) ||
        (options.autoDoor && frame==2) || (options.autoCloseDoor && frame==options.doorCloseFrame);
}

void Game::applyAraneaContactScenario() {
    if(options.autoCatch && frame==2 && encounter.alive() && world.level()==Level::Zero) {
        const Vec3 front=normalize(Vec3{forward.x,0,forward.z});
        double tx=player.x+front.x*1.35,tz=player.z+front.z*1.35;
        if(world.blocked(tx,tz,Threat::RADIUS)) { tx=player.x; tz=player.z; }
        threat.resetAt(tx,tz,std::atan2(-front.x,front.z));
    }
}

void Game::applyMouseContactScenario(ThreatPlayer& perceivedPlayer) {
    if(options.mouseFixture && frame==2 && mouse.active) {
        player.x=mouse.x;player.z=mouse.z;perceivedPlayer.x=player.x;perceivedPlayer.z=player.z;
    }
}

void Game::applyMousePreviewScenario() {
    if(options.mousePreview && frame==1 && mouse.active) {
        player.x=mouse.x+1;player.z=mouse.z+1;
        input.setLook(-135,-48);player.eyeHeight=1.08f;
    }
}

void Game::applyPartygoerScenario() {
    if(options.partyPreview && frame==0 && world.isFun() && !options.noThreat) {
        auto& a=partygoers.actors[0];a.active=true;a.x=player.x;a.z=player.z-3;a.yaw=PI;
    }
    if(options.autoCatch && frame==2 && world.isFun() && !options.noThreat) {
        auto& a=partygoers.actors[0];a.active=true;a.x=player.x;a.z=player.z+.4;
    }
}

void Game::applyDarknessScenario() {
    if(options.darknessPreview && frame==0 && world.isParking() && !options.noThreat) {
        auto& eyes=darkness.eyes[0];eyes.active=true;eyes.x=player.x;eyes.z=player.z-8;
    }
    if(options.autoCatch && frame==2 && world.isParking() && !options.noThreat) {
        player.x=3.75;player.z=-3.75;world.update(player.x,player.z);
        auto& eyes=darkness.eyes[0];eyes.active=true;eyes.x=player.x;eyes.z=player.z+.4;
    }
}

bool Game::usesFixedTimeStep() const {
    return !options.demoControlsCheck.empty() || !options.presentationCheck.empty() || options.stressCycles ||
           (options.endingPreview && options.hidden) ||
           options.levelMenuCheck ||
           options.controlHoldCheck ||
           options.controlPauseCheck ||
           options.controlTapCheck ||
           options.autoReadObservation ||
           options.darknessPreview ||
           options.autoBreakers ||
           options.partyPreview ||
           options.autoWalk ||
           options.autoCrouch ||
           options.autoDoor ||
           options.autoExit ||
           options.autoCatch ||
           options.autoJumpscare ||
           options.autoKey ||
           options.autoHide ||
           options.mouseFixture ||
           options.guideCheck ||
           options.forcedThreat;
}

Vec3 Game::inspectionMovementInput() const {
    if(options.presentationCheck=="camera" && frame>=3 && frame<6)return {0,1,1};
    const auto& mode=options.demoControlsCheck;
    if(!mode.empty()) {
        const int stop=mode=="camera-return"?438:360;
        if(mode.rfind("camera-",0)!=0 || frame<3 || frame>=stop)return {};
        // Move vertically first, then along the same look-based path as WASD.
        if(frame<5)return {0,1,0};
        return {0,0,1};
    }
    return {float(input.down(GLFW_KEY_D))-float(input.down(GLFW_KEY_A)),
            float(input.down(GLFW_KEY_SPACE))-float(input.down(GLFW_KEY_LEFT_CONTROL)||input.down(GLFW_KEY_RIGHT_CONTROL)),
            float(input.down(GLFW_KEY_W))-float(input.down(GLFW_KEY_S))};
}

bool Game::inspectionMoveFast() const {
    return !options.demoControlsCheck.empty() || input.down(GLFW_KEY_LEFT_SHIFT) || input.down(GLFW_KEY_RIGHT_SHIFT);
}

void Game::applyPresentationScenario() {
    const auto& mode=options.presentationCheck;
    if(mode.empty())return;
    auto& probe=presentationProbe;
    auto check=[&](bool passed,const char* message) {
        if(!passed)throw std::runtime_error("Presentation check ("+mode+"): "+message);
    };
    auto equal=[](double a,double b){return std::abs(a-b)<.00001;};
    auto verified=[&]() {
        if(!probe.verified){probe.verified=true;log("Presentation verified: "+mode);}
    };
    // The dedicated settings fixture intentionally differs from every preset.
    check(equal(settings.ambientOcclusion,0)&&equal(settings.brightness,.7f)&&
        equal(settings.blur,1.2f)&&equal(settings.fov,92)&&equal(settings.headBob,.8f),
        "presentation must not overwrite ordinary player settings");
    const bool menuStart=mode=="menu-key"||mode=="menu-click";
    const int menuFrame=options.demo?3:2;
    if(menuStart && frame==menuFrame) {
        check(flow.is(Screen::Menu)&&!presentationActive,"fixture must start in the ordinary menu");
        if(mode=="menu-key")simulateKey(GLFW_KEY_P);
        else {
            int w,h;glfwGetFramebufferSize(window,&w,&h);
            const float boxW=std::min(700.f,float(w)-40);
            scriptedMenuClick=std::array<double,2>{(w-boxW)*.5+boxW-140,(h-550)*.5+73};
            Input::button(window,GLFW_MOUSE_BUTTON_LEFT,GLFW_PRESS,0);
        }
    }
    const int first=menuStart?menuFrame+1:1;
    if(frame==first) {
        check(presentationActive&&flow.gameplay()&&!input.paused&&!flow.guidePending,
            "presentation must show the scene immediately without a menu or automatic guide");
        check(world.level()==Level::Zero&&world.seed()==PRESENTATION_SEED&&
            equal(player.x,CELL*.5)&&equal(player.z,CELL*.5),"presentation must use the reproducible scene");
        check(equal(input.yaw,PRESENTATION_YAW)&&equal(input.pitch,PRESENTATION_PITCH)&&
            equal(renderFov,PRESENTATION_FOV),"presentation must use the reproducible view");
        check(equal(actualAOAmount,PRESENTATION_AO)&&aoPasses>0,
            "the first presentation frame must actually render AO even when saved AO is zero");
        probe.captured=true;
        probe.playerX=player.x;probe.playerZ=player.z;probe.elapsed=elapsed;
        probe.threatX=threat.x();probe.threatZ=threat.z();probe.stamina=player.stamina.value;
        probe.bob=player.bobOffset;probe.animation=threatAnimation;
        if(mode=="direct"||mode=="ao-on"||menuStart)verified();
    }
    if(probe.captured&&presentationActive) {
        check(equal(player.x,probe.playerX)&&equal(player.z,probe.playerZ)&&equal(player.stamina.value,probe.stamina),
            "presentation must freeze player movement and stamina");
        check(equal(threat.x(),probe.threatX)&&equal(threat.z(),probe.threatZ)&&equal(threatAnimation,probe.animation),
            "presentation must freeze actor motion");
        check(equal(elapsed,probe.elapsed)&&equal(player.bobOffset,probe.bob),
            "presentation must freeze shader animation and head bob for comparison");
    }
    if((mode=="ao-off"||mode=="ao-toggle")&&frame==3) {
        probe.aoPasses=aoPasses;simulateKey(GLFW_KEY_G);
    }
    if((mode=="ao-off"||mode=="ao-toggle")&&frame==4) {
        check(actualAOAmount==0&&aoPasses==probe.aoPasses,"G must disable the actual presentation AO pass");
        if(mode=="ao-off")verified();
    }
    if(mode=="ao-toggle"&&frame==5)simulateKey(GLFW_KEY_G);
    if(mode=="ao-toggle"&&frame==6) {
        check(equal(actualAOAmount,PRESENTATION_AO)&&aoPasses>probe.aoPasses,"G must restore the presentation AO intensity");verified();
    }
    if(mode=="camera") {
        if(frame==2||frame==6)simulateKey(GLFW_KEY_F4);
        if(frame==3){check(inspectionCamera.active&&presentationActive,"F4 must enter inspection within presentation");input.setLook(0,0);}
        if(frame==6)check(inspectionCamera.y>player.eyeHeight,"inspection must move independently during presentation");
        if(frame==7) {
            check(!inspectionCamera.active&&presentationActive&&equal(input.yaw,PRESENTATION_YAW)&&equal(input.pitch,PRESENTATION_PITCH),
                "F4 return must restore the fixed shot without starting gameplay");verified();
        }
    }
    if(mode=="pause") {
        if(frame==2||frame==5||frame==6)simulateKey(GLFW_KEY_ESCAPE);
        if(frame==3)simulateKey(GLFW_KEY_O);
        if(frame==4)check(flow.is(Screen::Settings)&&input.paused&&presentationActive,"settings must pause but retain presentation");
        if(frame==7){check(flow.gameplay()&&!input.paused&&presentationActive,"resume must return to the presentation");verified();}
    }
    if(mode=="play"&&frame==2)simulateKey(GLFW_KEY_ENTER);
    if(mode=="play"&&frame==3) {
        check(!presentationActive&&!inspectionCamera.active&&flow.gameplay()&&elapsed>probe.elapsed,
            "Enter must begin normal gameplay and resume simulation");
        check(actualAOAmount==0&&equal(renderFov,settings.fov),"normal rendering must use the retained player settings");verified();
    }
    if(mode=="reset"&&frame==2)simulateKey(GLFW_KEY_R);
    if(mode=="reset"&&frame==3){check(!presentationActive&&!inspectionCamera.active,"reset must end presentation");verified();}
    if(mode=="level") {
        if(frame==2)simulateKey(GLFW_KEY_ESCAPE);
        if(frame==3||frame==4||frame==6)simulateKey(GLFW_KEY_DOWN);
        if(frame==5||frame==7)simulateKey(GLFW_KEY_ENTER);
        if(frame==8){check(!presentationActive&&world.isFun()&&flow.gameplay(),"level selection must end presentation");verified();}
    }
    if(mode=="ending"&&frame==2)simulateKey(GLFW_KEY_F9);
    if(mode=="ending"&&frame==3){check(!presentationActive&&!inspectionCamera.active&&flow.cinematic(),"ending preview must end presentation");verified();}
}

void Game::applyDemoControlsScenario() {
    const auto& mode=options.demoControlsCheck;
    if(mode.empty())return;
    auto& probe=demoControlsProbe;
    auto check=[&](bool passed,const char* message) {
        if(!passed)throw std::runtime_error("Demo controls check ("+mode+"): "+message);
    };
    auto equal=[](double a,double b){return std::abs(a-b)<.00001;};
    auto verified=[&]() {
        if(!probe.verified){probe.verified=true;log("Demo controls verified: "+mode);}
    };
    if(mode.rfind("ao-",0)==0) {
        if(frame==2) {
            probe.aoAmount=settings.ambientOcclusion;probe.aoPasses=aoPasses;
            if(mode!="ao-on")simulateKey(GLFW_KEY_G);
        }
        if(frame==3) {
            if(mode=="ao-zero") {
                check(probe.aoAmount==0,"fixture must begin with zero AO intensity");
                check(equal(actualAOAmount,.65f)&&aoPasses>probe.aoPasses,"G from zero must enable the AO pass at default intensity");
                verified();
            } else if(mode=="ao-on") {
                check(equal(actualAOAmount,probe.aoAmount)&&actualAOAmount>0,"baseline AO must reach the renderer");
                verified();
            } else {
                check(actualAOAmount==0&&aoPasses==probe.aoPasses,"G must skip the AO pass and remove its composite contribution");
                if(mode=="ao-off")verified();
            }
        }
        if(mode=="ao-restore"&&frame==4)simulateKey(GLFW_KEY_G);
        if(mode=="ao-restore"&&frame==5) {
            check(equal(actualAOAmount,probe.aoAmount)&&aoPasses>probe.aoPasses,"second G must restore the previous positive intensity and render AO");
            verified();
        }
        return;
    }
    if(mode=="camera-menu"||mode=="camera-capture") {
        if(frame==5)simulateKey(GLFW_KEY_F4);
        if(frame==6){check(!inspectionCamera.active,"F4 must be ignored outside live gameplay");verified();}
        return;
    }
    if(mode=="camera-return"&&frame==1)
        check(world.shiftBehind(player.x,player.z,1,0),"roundtrip fixture must include a shifted original chunk");
    if(frame==2)simulateKey(GLFW_KEY_F4);
    if(frame==3) {
        check(inspectionCamera.active,"F4 must detach the inspection camera");
        probe.captured=true;
        probe.playerX=player.x;probe.playerZ=player.z;probe.stamina=player.stamina.value;
        probe.yaw=inspectionCamera.playerYaw;probe.pitch=inspectionCamera.playerPitch;
        probe.threatX=threat.x();probe.threatZ=threat.z();probe.animation=threatAnimation;
        probe.elapsed=elapsed;probe.bob=player.bobOffset;probe.partyTravel=partygoers.travelled;
        probe.key=world.keyCollected();probe.circuits=world.breakersPowered();probe.presents=world.presentsOpened();probe.doors=doorsUsed;
        probe.cameraX=inspectionCamera.x;probe.cameraY=inspectionCamera.y;probe.cameraZ=inspectionCamera.z;
        for(const auto& entry:world.chunks())probe.chunkRevisions.emplace(entry.first,entry.second.revision);
    }
    if(probe.captured&&inspectionCamera.active) {
        check(equal(player.x,probe.playerX)&&equal(player.z,probe.playerZ)&&equal(player.stamina.value,probe.stamina),"inspection must preserve player position and stamina");
        check(equal(threat.x(),probe.threatX)&&equal(threat.z(),probe.threatZ)&&equal(threatAnimation,probe.animation)&&equal(partygoers.travelled,probe.partyTravel),"inspection must freeze actor movement and animation");
        check(world.keyCollected()==probe.key&&world.breakersPowered()==probe.circuits&&world.presentsOpened()==probe.presents&&doorsUsed==probe.doors,"inspection must preserve progression");
        check(equal(elapsed,probe.elapsed)&&equal(player.bobOffset,probe.bob),"inspection must freeze shader time and player head bob");
        check(world.chunks().size()<=25&&world.preparedChunks().size()<=24,"inspection streaming must remain bounded");
    }
    if(frame==5)input.setLook(0,0); // Different from saved player look; return must restore it.
    if(mode=="camera-move"&&frame==360) {
        check(inspectionCamera.y>probe.cameraY+.5f,"Space input must move the camera vertically");
        check(std::hypot(inspectionCamera.x-probe.playerX,inspectionCamera.z-probe.playerZ)>CHUNK_SIZE,"WASD must cross a streamed chunk boundary");
        check(!(origin==chunkAt(player.x,player.z))&&origin==chunkAt(inspectionCamera.x,inspectionCamera.z),"render origin must follow the inspection camera");
        verified();
    }
    if(mode=="camera-pause") {
        if(frame==8||frame==12)simulateKey(GLFW_KEY_ESCAPE);
        if(frame==9) {
            check(input.paused,"Escape must pause inspection");
            probe.cameraX=inspectionCamera.x;probe.cameraY=inspectionCamera.y;probe.cameraZ=inspectionCamera.z;
        }
        if(frame>=10&&frame<=12)check(equal(inspectionCamera.x,probe.cameraX)&&equal(inspectionCamera.y,probe.cameraY)&&equal(inspectionCamera.z,probe.cameraZ),"paused inspection must ignore held movement");
        if(frame==14){check(!input.paused&&inspectionCamera.x>probe.cameraX,"resuming must restore inspection movement");verified();}
    }
    if(mode=="camera-return"&&frame==438) {
        check(std::hypot(inspectionCamera.x-probe.playerX,inspectionCamera.z-probe.playerZ)>3*CHUNK_SIZE,"roundtrip must travel beyond the original resident world");
        simulateKey(GLFW_KEY_F4);
    }
    if(mode=="camera-return"&&frame==439) {
        check(!inspectionCamera.active&&equal(player.x,probe.playerX)&&equal(player.z,probe.playerZ),"F4 return must preserve player position");
        check(equal(input.yaw,probe.yaw)&&equal(input.pitch,probe.pitch),"F4 return must restore player look");
        check(world.chunks().count(chunkAt(player.x,player.z))>0,"return must restore player collision chunks");
        check(world.chunks().size()==probe.chunkRevisions.size(),"return must restore the original resident world");
        for(const auto& entry:world.chunks()) {
            const auto previous=probe.chunkRevisions.find(entry.first);
            check(previous!=probe.chunkRevisions.end()&&previous->second==entry.second.revision,"return must preserve every original chunk revision, including shifted geometry");
        }
        verified();
    }
    if(mode=="camera-reset"&&frame==10)simulateKey(GLFW_KEY_R);
    if(mode=="camera-reset"&&frame==11) {
        check(!inspectionCamera.active&&equal(player.x,CELL*.5)&&equal(player.z,CELL*.5),"reset must leave inspection at the entrance");
        check(equal(input.yaw,-90)&&equal(input.pitch,-3),"reset must restore the entrance look");verified();
    }
    if(mode=="camera-level") {
        if(frame==10)simulateKey(GLFW_KEY_ESCAPE);
        if(frame==11||frame==12||frame==14)simulateKey(GLFW_KEY_DOWN);
        if(frame==13||frame==15)simulateKey(GLFW_KEY_ENTER);
        if(frame==16){check(!inspectionCamera.active&&world.isFun()&&flow.gameplay(),"level selection must leave inspection and enter the new level");verified();}
    }
    if(mode=="camera-ending"&&frame==10)simulateKey(GLFW_KEY_F9);
    if(mode=="camera-ending"&&frame==11){check(!inspectionCamera.active&&flow.cinematic(),"ending preview must leave inspection");verified();}
}

} // namespace br::app
