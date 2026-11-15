#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>
#include "../common/audio.h"
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void check(bool condition,const std::string& message) {
    if(!condition)throw std::runtime_error(message+": "+SDL_GetError());
}
struct SDLRuntime {
    SDLRuntime() {
        SDL_SetMainReady();
        check(SDL_setenv("SDL_AUDIODRIVER","dummy",1)==0,"select headless audio driver");
    }
    ~SDLRuntime() { SDL_Quit(); }
};
void effectsLifecycle() {
    // A separate reference represents the soundtrack's subsystem ownership.
    // Releasing or muting effects must not shut that shared subsystem down.
    check(SDL_InitSubSystem(SDL_INIT_AUDIO)==0,"initialize independent audio owner");
    {
        br::Audio audio;
        check(!audio.available(),"new effects player is stopped");
        audio.start();
        check(audio.available(),"start effects output");
        audio.start();
        check(audio.available(),"starting twice is harmless");
        for(int frame=0;frame<12;++frame) {
            audio.update(.016f,.15f,false,false);
            SDL_Delay(4);
        }
        audio.setPaused(true);
        audio.update(.016f,0,false,false,br::Surface::Carpet,true);
        check(audio.available(),"pause retains the output device");
        audio.reset(1234);
        audio.setPaused(false);
        audio.update(.016f,.15f,false,true);
        check(audio.available(),"resume after resetting a paused scene");
        audio.setPaused(true);
        audio.play(br::SoundKind::Jumpscare,{0,0,0});
        check(audio.available(),"capture can take ownership from a paused scene");
        audio.update(.016f,0,false,false);
        audio.stop();
        check(!audio.available(),"mute closes the effects device");
        check(SDL_WasInit(SDL_INIT_AUDIO)==SDL_INIT_AUDIO,"mute retains another owner's subsystem");
        audio.stop();
        check(SDL_WasInit(SDL_INIT_AUDIO)==SDL_INIT_AUDIO,"repeated stop does not release another reference");
        audio.start();
        check(audio.available(),"unmute opens a fresh effects device");
    }
    check(SDL_WasInit(SDL_INIT_AUDIO)==SDL_INIT_AUDIO,"destructor retains another owner's subsystem");
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    check(SDL_WasInit(SDL_INIT_AUDIO)==0,"all audio subsystem references were released");
}
void unavailableDevice() {
    check(SDL_setenv("SDL_AUDIODRIVER","backrooms-nonexistent-test-driver",1)==0,"select an unavailable driver");
    {
        br::Audio audio;
        audio.start();
        check(!audio.available(),"unavailable driver leaves effects disabled");
        audio.update(.016f,.15f,false,false);
        audio.play(br::SoundKind::Jumpscare,{0,0,0});
        audio.stop();
    }
    check(SDL_WasInit(SDL_INIT_AUDIO)==0,"failed startup does not retain audio initialization");
}
}
int main() {
    try {
        SDLRuntime runtime;
        effectsLifecycle();
        unavailableDevice();
        std::cout<<"SDL effects tests passed: start, pause/resume, reset, capture, mute, reopen, shared ownership and unavailable driver.\n";
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<"SDL effects test failure: "<<error.what()<<'\n';
        return 1;
    }
}
