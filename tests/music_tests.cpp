#include "../common/music.h"
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>

namespace {
void check(bool condition,const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
struct TemporaryMusic {
    std::filesystem::path path;
    TemporaryMusic() {
        const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count();
        path=std::filesystem::temp_directory_path()/(L"Backrooms music \u0442\u0435\u0441\u0442 "+std::to_wstring(stamp)+L".wav");
    }
    ~TemporaryMusic() { std::error_code error; std::filesystem::remove(path,error); }
};
void writeSilentWav(const std::filesystem::path& path) {
    // Silence is deliberate: exercise the real device lifecycle at nonzero
    // volume without playing a test tone to the user's speakers.
    std::ofstream output(path,std::ios::binary);
    const auto write16=[&](std::uint16_t value) {
        const char bytes[]={char(value),char(value>>8)};
        output.write(bytes,2);
    };
    const auto write32=[&](std::uint32_t value) {
        const char bytes[]={char(value),char(value>>8),char(value>>16),char(value>>24)};
        output.write(bytes,4);
    };
    constexpr std::uint32_t rate=22050,samples=rate*2;
    output.write("RIFF",4); write32(36+samples*2); output.write("WAVEfmt ",8);
    write32(16); write16(1); write16(1); write32(rate); write32(rate*2);
    write16(2); write16(16); output.write("data",4); write32(samples*2);
    for (std::uint32_t i=0;i<samples;++i) write16(0);
    check(bool(output),"temporary WAV was written");
}
void tick(br::BackgroundMusic& music,int milliseconds,bool paused=false,bool muted=false,float volume=.4f) {
    for (int elapsed=0;elapsed<milliseconds;elapsed+=25) {
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        music.update(paused,muted,volume,.025f);
    }
}
void missingAndCorrupt() {
    br::BackgroundMusic music;
    check(!music.available() && music.status()==br::MusicStatus::Missing,"new player has no track");
    TemporaryMusic temporary;
    check(!music.load(temporary.path),"absent optional track is not an error");
    check(music.error().empty() && music.status()==br::MusicStatus::Missing,"missing file remains quiet");
    for (int i=0;i<500;++i) music.update(false,false,.5f,.016f);
    check(music.error().empty(),"missing track is not retried on every frame");
    { std::ofstream invalid(temporary.path,std::ios::binary); invalid<<"not a playable audio file"; }
    check(!music.load(temporary.path),"corrupt file is rejected");
    check(!music.error().empty() && music.status()==br::MusicStatus::Failed,"corrupt file has useful diagnostic");
    const auto error=music.error();
    for (int i=0;i<500;++i) music.update(false,false,.5f,.016f);
    check(music.error()==error && !music.available(),"failed device is closed with one stable diagnostic");
}
void nativeLifecycle() {
    TemporaryMusic temporary;
    writeSilentWav(temporary.path);
    br::BackgroundMusic music;
    auto missingMp3=temporary.path;
    missingMp3.replace_extension(L".mp3");
    const bool loaded=music.load(missingMp3);
    check(loaded,"Unicode/spaced filename and WAV fallback load: "+music.error());
    check(music.available() && music.status()==br::MusicStatus::Ready,"load never autoplays");
    check(music.durationMilliseconds()>=1950 && music.durationMilliseconds()<=2050,"native duration is in milliseconds");
    tick(music,100,true);
    check(music.positionMilliseconds()==0 && music.status()==br::MusicStatus::Ready,"menu does not start soundtrack");
    tick(music,400);
    check(music.status()==br::MusicStatus::Playing && music.positionMilliseconds()>100,"real device advances during gameplay");
    music.update(true,false,.4f,.025f);
    check(music.status()==br::MusicStatus::Paused,"pause menu pauses native playback");
    const auto pausedAt=music.positionMilliseconds();
    tick(music,350,true);
    check(music.positionMilliseconds()==pausedAt,"paused position is preserved");
    tick(music,350);
    check(music.positionMilliseconds()>pausedAt+100,"resuming continues from same position");
    music.update(false,true,.4f,.025f);
    check(music.status()==br::MusicStatus::Paused,"M-key mute suspends music");
    const auto mutedAt=music.positionMilliseconds();
    tick(music,100,false,true);
    check(music.positionMilliseconds()==mutedAt,"mute does not rewind track");
    tick(music,2500);
    check(music.available() && music.loopCount()>=1 && music.status()==br::MusicStatus::Playing,"track loops after natural end");
    music.update(false,false,std::numeric_limits<float>::quiet_NaN(),.025f);
    check(music.status()==br::MusicStatus::Paused,"invalid volume fails silent");
    music.update(false,false,2,.025f);
    check(music.status()==br::MusicStatus::Playing,"volume clamps to valid range");
    music.stop();
    check(!music.available() && music.error().empty(),"stop closes device cleanly");
    std::error_code error;
    check(std::filesystem::remove(temporary.path,error),"device released the open audio file");
}
}
int main(int argc,char** argv) {
    try {
        if (argc==3 && std::string(argv[1])=="--check-file") {
            br::BackgroundMusic supplied;
            const bool loaded=supplied.load(std::filesystem::u8path(argv[2]));
            check(loaded,"supplied music codec load: "+supplied.error());
            supplied.update(true,true,0,.1f);
            check(supplied.available() && supplied.status()==br::MusicStatus::Ready,
                  "supplied music remains silent in the menu");
            std::cout<<"Supplied track opened successfully, duration "<<supplied.durationMilliseconds()
                     <<" ms; no playback started.\n";
            return 0;
        }
        nativeLifecycle();
        missingAndCorrupt();
        std::cout<<"Music tests passed: missing/corrupt input, Unicode WAV fallback, pause/resume, mute, loop and clean close.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr<<"Music test failure: "<<error.what()<<'\n';
        return 1;
    }
}
