#include "game_internal.h"

namespace br::app {
namespace fs = std::filesystem;

void Game::simulateKey(int key) {
    Input::key(window,key,0,GLFW_PRESS,0);
}

void Game::applyScenarioInput() {
    scriptedMenuClick.reset();
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
    return options.stressCycles ||
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

} // namespace br::app
