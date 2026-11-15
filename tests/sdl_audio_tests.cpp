#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <SDL.h>
#include "../common/audio.h"
#include "../common/music.h"
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void check(bool condition,const std::string& message) {
    if(!condition)throw std::runtime_error(message+": "+SDL_GetError());
}
struct SDLRuntime {
    explicit SDLRuntime(bool useDummy=true) {
        SDL_SetMainReady();
        if(useDummy)check(SDL_setenv("SDL_AUDIODRIVER","dummy",1)==0,"select headless audio driver");
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
int systemDeviceCheck() {
    // Probe the exact format before exercising the game. Hosted Mac runners
    // may expose no physical/default output; that is unavailable evidence,
    // rather than a passing audio test or a game-backend failure.
    if(SDL_InitSubSystem(SDL_INIT_AUDIO)<0) {
        std::cout<<"SKIP system audio: initialization unavailable: "<<SDL_GetError()<<'\n';
        return 77;
    }
    const char* currentDriver=SDL_GetCurrentAudioDriver();
    const std::string driver=currentDriver?currentDriver:"unknown";
    const int outputDevices=SDL_GetNumAudioDevices(0);
    std::cout<<"System audio driver: "<<driver<<"; enumerated outputs: "<<outputDevices<<'\n';
    if(driver=="dummy" || driver=="disk") {
        std::cout<<"SKIP system audio: a synthetic SDL driver is selected; no hardware check was performed.\n";
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return 77;
    }
    SDL_AudioSpec desired{},obtained{};
    desired.freq=br::AUDIO_SAMPLE_RATE;
    desired.format=AUDIO_S16SYS;
    desired.channels=2;
    desired.samples=512;
    const auto probe=SDL_OpenAudioDevice(nullptr,0,&desired,&obtained,0);
    if(!probe) {
        std::cout<<"SKIP system audio: default output unavailable: "<<SDL_GetError()<<'\n';
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return 77;
    }
    // The raw probe starts paused and queues nothing, so it remains silent.
    SDL_CloseAudioDevice(probe);
    SDL_QuitSubSystem(SDL_INIT_AUDIO);

    br::BackgroundMusic music;
    br::Audio effects;
    const auto loadMusic=[&](const char* operation) {
        const bool loaded=music.load();
        check(loaded,std::string(operation)+": "+music.error());
    };
    effects.setVolume(0);
    effects.setPaused(true);
    loadMusic("standalone packaged WAV load");
    check(music.available(),"standalone soundtrack device opened");
    music.update(true,true,0,0);
    music.stop();
    effects.start();
    check(effects.available(),"standalone effects device opened");
    effects.stop();

    // Each game backend can open alone. Any failure from here is a real
    // coexistence failure, and must not be disguised as a missing-host skip.
    loadMusic("open soundtrack before effects");
    music.update(true,true,0,0);
    effects.start();
    check(effects.available() && music.available(),"simultaneous soundtrack and effects devices opened");
    std::cout<<"Both outputs available: effects=yes; music=yes; sound output muted.\n";
    effects.stop();
    check(music.available() && SDL_WasInit(SDL_INIT_AUDIO)==SDL_INIT_AUDIO,
          "closing effects retains soundtrack audio ownership");
    effects.start();
    check(effects.available(),"reopen effects while soundtrack remains loaded");
    music.stop();
    // Exercise the surviving effects queue with master volume zero.
    effects.setPaused(false);
    effects.update(.016f,0,false,false);
    check(effects.available(),"effects queue remains usable after soundtrack closes");
    loadMusic("open soundtrack while effects remain active");
    music.update(true,true,0,0);
    check(effects.available() && music.available(),"both outputs coexist in the reverse opening order");
    music.stop();
    effects.stop();
    check(SDL_WasInit(SDL_INIT_AUDIO)==0,"both outputs release their subsystem references");
    std::cout<<"PASS system audio: simultaneous music/effects, both opening orders, independent close/reopen, silent cleanup.\n";
    return 0;
}
}
int main(int argc,char** argv) {
    try {
        const bool systemDevice=argc==2 && std::string(argv[1])=="--system-device-check";
        if(argc!=1 && !systemDevice)throw std::runtime_error("Usage: sdl_audio_tests [--system-device-check]");
        SDLRuntime runtime(!systemDevice);
        if(systemDevice)return systemDeviceCheck();
        effectsLifecycle();
        unavailableDevice();
        std::cout<<"SDL effects tests passed: start, pause/resume, reset, capture, mute, reopen, shared ownership and unavailable driver.\n";
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<"SDL effects test failure: "<<error.what()<<'\n';
        return 1;
    }
}
