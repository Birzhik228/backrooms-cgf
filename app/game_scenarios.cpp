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

} // namespace br::app
