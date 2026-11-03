#include "game_internal.h"

namespace br::app {
namespace fs = std::filesystem;

void Game::saveSettings() {
    settings.sanitize(); input.sensitivity = settings.sensitivity;
    audio.setVolume(settings.volume);
    glfwSwapInterval(options.hidden ? 0 : int(settings.vsync));
    if ((!options.hidden || !options.settingsFile.empty()) && !settings.save(settingsPath)) {
        notice = "SETTINGS ACTIVE / COULD NOT SAVE FILE"; noticeTime = 4;
    }
}

void Game::handleMenus() {
    bool resumeAction = false, newSeedAction = false, resetAction = false, quitAction = false;
    bool escapeHandled = false;
    const bool selectorConsumes=flow.is(Screen::LevelSelect);
    if(flow.is(Screen::LevelSelect)) {
        int fw,fh,ww,wh;glfwGetFramebufferSize(window,&fw,&fh);glfwGetWindowSize(window,&ww,&wh);
        double mx,my;glfwGetCursorPos(window,&mx,&my);
        mx*=double(fw)/std::max(1,ww);my*=double(fh)/std::max(1,wh);
        if(scriptedMenuClick){mx=(*scriptedMenuClick)[0];my=(*scriptedMenuClick)[1];}
        if(input.hit(GLFW_KEY_UP))levelMenuSelection=(levelMenuSelection+3)%4;
        if(input.hit(GLFW_KEY_DOWN))levelMenuSelection=(levelMenuSelection+1)%4;
        bool activate=input.hit(GLFW_KEY_ENTER);
        if(input.clicked) {
            const int row=LevelMenuLayout(fw,fh).rowAt(mx,my);
            if(row>=0){levelMenuSelection=row;activate=true;}
        }
        if(input.hit(GLFW_KEY_ESCAPE)||(activate&&levelMenuSelection==3)) {
            setScreen(Screen::Menu);log("Level menu closed; returned to pause menu.");
        } else if(activate)enterLevel(Level(levelMenuSelection),true);
    }
    if(selectorConsumes){input.pressed.fill(false);input.clicked=false;escapeHandled=true;}
    const bool guideWasOpen=flow.is(Screen::Controls);
    if(flow.is(Screen::Controls)) {
        int fw,fh,ww,wh;glfwGetFramebufferSize(window,&fw,&fh);glfwGetWindowSize(window,&ww,&wh);
        double mx,my;glfwGetCursorPos(window,&mx,&my);
        mx*=double(fw)/std::max(1,ww);my*=double(fh)/std::max(1,wh);
        const GuideLayout layout(fw,fh);
        if(options.guideCheck && options.guideClick && (frame==5 || frame==7 || frame==9)) {
            mx=layout.left+760*layout.scale;my=layout.top+506*layout.scale;
        }
        const bool next=input.hit(GLFW_KEY_ENTER) || input.hit(GLFW_KEY_RIGHT) ||
            (input.clicked && layout.contains(mx,my,652,484,216,44));
        const bool back=input.hit(GLFW_KEY_LEFT) ||
            (input.clicked && layout.contains(mx,my,32,486,126,40));
        const bool dismiss=input.hit(GLFW_KEY_ESCAPE) || input.hit(GLFW_KEY_H) ||
            (input.clicked && layout.contains(mx,my,838,14,38,32));
        if(dismiss || (next && guidePage==2)) {
            setScreen(Screen::Gameplay);log("Controls popup closed.");
        } else if(next) { ++guidePage;log("Guide page: "+std::to_string(guidePage+1)); }
        else if(back && guidePage>0) { --guidePage;log("Guide page: "+std::to_string(guidePage+1)); }
        // Modal input must never also activate a menu or gameplay action.
        input.pressed.fill(false);input.clicked=false;escapeHandled=true;
    }
    if (input.paused && !encounter.scaring() && !guideWasOpen && !selectorConsumes) {
        int menuW, menuH, windowW, windowH;
        glfwGetFramebufferSize(window, &menuW, &menuH);
        glfwGetWindowSize(window, &windowW, &windowH);
        double mouseX, mouseY; glfwGetCursorPos(window, &mouseX, &mouseY);
        mouseX *= double(menuW)/std::max(1,windowW); mouseY *= double(menuH)/std::max(1,windowH);
        if(scriptedMenuClick) { mouseX=(*scriptedMenuClick)[0]; mouseY=(*scriptedMenuClick)[1]; }
        const float boxW = std::min(700.f, float(menuW)-40), boxH = 550;
        const float left=(menuW-boxW)*.5f, top=(menuH-boxH)*.5f;
        if (flow.is(Screen::Settings)) {
            if (input.hit(GLFW_KEY_UP)) settingSelection=(settingSelection+13)%14;
            if (input.hit(GLFW_KEY_DOWN)) settingSelection=(settingSelection+1)%14;
            int change = input.hit(GLFW_KEY_LEFT) ? -1 : (input.hit(GLFW_KEY_RIGHT) || input.hit(GLFW_KEY_ENTER)) ? 1 : 0;
            bool activate = input.hit(GLFW_KEY_ENTER);
            if (input.clicked && mouseX>=left+28 && mouseX<=left+boxW-28) {
                int row=int(std::floor((mouseY-top-112)/25));
                if (row>=0 && row<14) {
                    settingSelection=row;
                    activate = row>=12;
                    if (row==7 || row==10) change=1;
                    if (mouseX>left+boxW-150) change=mouseX>left+boxW-85 ? 1 : -1;
                }
            }
            if (change && settingSelection<12) {
                switch(settingSelection) {
                case 0: settings.sensitivity+=change*.01f; break;
                case 1: settings.volume+=change*.05f; break;
                case 2: settings.brightness+=change*.05f; break;
                case 3: settings.fov+=change*2.f; break;
                case 4: settings.headBob+=change*.1f; break;
                case 5: settings.blur+=change*.1f; break;
                case 6: settings.vsync=!settings.vsync; break;
                case 7: settings.musicEnabled=!settings.musicEnabled; break;
                case 8: settings.musicVolume+=change*.05f; break;
                case 9: settings.ambientOcclusion+=change*.1f; break;
                case 10: settings.showFPS=!settings.showFPS; break;
                case 11: {
                    constexpr std::array<int,7> caps={30,60,90,120,144,240,0};
                    auto it=std::find(caps.begin(),caps.end(),settings.maxFps);
                    int index=it==caps.end()?1:int(it-caps.begin());
                    settings.maxFps=caps[(index+change+7)%7];break;
                }
                }
                saveSettings(); fieldOfView=settings.fov;
            }
            if (activate && settingSelection==12) { settings=Settings{}; saveSettings(); fieldOfView=settings.fov; }
            if ((activate && settingSelection==13) || input.hit(GLFW_KEY_ESCAPE)) {
                setScreen(Screen::Menu); escapeHandled=true; saveSettings();
            }
        } else {
            if (input.hit(GLFW_KEY_UP)) menuSelection=(menuSelection+5)%6;
            if (input.hit(GLFW_KEY_DOWN)) menuSelection=(menuSelection+1)%6;
            bool activate=input.hit(GLFW_KEY_ENTER);
            if(input.clicked && mouseX>=left+32 && mouseX<=left+boxW-32) {
                const double rowY=mouseY-top-163;
                int row=int(std::floor(rowY/48));
                if(row>=0 && row<6 && rowY-row*48<43) {menuSelection=row;activate=true;}
            }
            if(input.hit(GLFW_KEY_O)) {setScreen(Screen::Settings);activate=false;}
            if(activate) {
                if(menuSelection==0) resumeAction=true;
                if(menuSelection==1) setScreen(Screen::Settings);
                if(menuSelection==2) {
                    setScreen(Screen::LevelSelect);levelMenuSelection=int(world.level());
                    flow.guidePending=false;player.motion.reset();++levelMenuVisits;
                    log("Level menu opened from pause menu.");
                    input.pressed.fill(false);input.clicked=false;escapeHandled=true;
                }
                if(menuSelection==3) newSeedAction=true;
                if(menuSelection==4) resetAction=true;
                if(menuSelection==5) quitAction=true;
            }
        }
    }
    // Input and focus management.
    if (input.hit(GLFW_KEY_ESCAPE) && !escapeHandled) {
        if(encounter.scaring()) encounter.skipScare();
        else if(!encounter.gameOver()) setScreen(input.paused ? Screen::Gameplay : Screen::Menu);
        if (input.paused) menuSelection=0;
        if (!input.paused) {
            flow.entered = true;
            flow.escaped = false;
        }
    }
    if ((resumeAction || (options.autoContinue && flow.escaped)) && input.paused && !flow.is(Screen::Settings)) {
        if(encounter.gameOver()) { freshRun(world.seed()); ++restarts; log(world.isParking()?"Restarted at Parking checkpoint.":world.isFun()?"Restarted at Level Fun checkpoint.":"Restarted after capture with the same seed."); }
        else if(!encounter.scaring()) setScreen(Screen::Gameplay);
        flow.entered = true;
        flow.escaped = false;
    }
    if ((quitAction || input.hit(GLFW_KEY_Q)) && input.paused && !flow.is(Screen::Settings)) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    if (!glfwGetWindowAttrib(window, GLFW_FOCUSED) &&
        !options.hidden && !options.tour && !input.paused) {
        setScreen(Screen::Menu);
        menuSelection=0;
    }
    if (input.hit(GLFW_KEY_F)) {
        flashlight = !flashlight;
    }
    if (input.hit(GLFW_KEY_C)) {
        clues = !clues;
    }
    if (input.hit(GLFW_KEY_TAB)) {
        showMap = !showMap;
    }
    if (input.hit(GLFW_KEY_H) && !input.paused && encounter.alive()) {
        setScreen(Screen::Controls);guidePage=0;player.motion.reset();
    }
    if (input.hit(GLFW_KEY_V)) {
        cameraView = !cameraView;
    }
    if (input.hit(GLFW_KEY_F1)) {
        diagnostics = !diagnostics;
    }
    if (input.hit(GLFW_KEY_F2)) {
        wireframe = !wireframe;
    }
    if (input.hit(GLFW_KEY_F3)) {
        shifting = !shifting;
    }
    if (input.hit(GLFW_KEY_M)) {
        muted = !muted;
        if (muted) {
            log("Lamp shadow cache updates: "+std::to_string(lampShadowUpdates));
            audio.stop();
        } else {
            audio.start();
            log(audio.available() ? "Audio output active." : "Audio output unavailable; use M to retry.");
        }
    }
    if (((input.hit(GLFW_KEY_R) && !flow.is(Screen::Settings)) || resetAction) && !encounter.scaring()) {
        if(encounter.gameOver()) {
            freshRun(world.seed());
            if(resetAction) { setScreen(Screen::Menu); flow.entered=false; }
            else ++restarts;
        }
        else resetPlayer();
    }
    if ((newSeedAction || input.hit(GLFW_KEY_N)) && input.paused && !flow.is(Screen::Settings) && !encounter.scaring()) {
        freshRun(world.seed()+104729);
        notice="NEW SEED / NEW BUILDING"; noticeTime=3;
    }
    if(flow.guidePending && flow.entered && !input.paused && encounter.alive() && !flow.is(Screen::LevelSelect)) {
        flow.guidePending=false;setScreen(Screen::Controls);guidePage=0;player.motion.reset();
        log("Controls popup opened.");
    }
    // Resume the mixer before any door, mouse, vocal or foot-contact event
    // consumes its one-shot state. Capture keeps its existing pause policy.
    scareSuspended=!options.hidden && !glfwGetWindowAttrib(window,GLFW_FOCUSED);
    audio.setPaused(input.paused && (!encounter.presenting() || scareSuspended || !scareConfig.audio));
    input.smooth(dt);
    if (options.tour && encounter.alive() && !input.paused && !player.hiding) {
        input.setLook(options.yaw + std::sin(elapsed * .38) * 40, -3 + std::sin(elapsed * .2) * 4);
    }
}

} // namespace br::app
