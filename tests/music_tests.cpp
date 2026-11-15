#include "../common/music.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>

namespace {
using Clock=std::chrono::steady_clock;
void check(bool condition,const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
std::uint64_t playedMilliseconds(const br::BackgroundMusic& music) {
    return static_cast<std::uint64_t>(music.loopCount())*music.durationMilliseconds()+
        music.positionMilliseconds();
}
std::string describe(const br::BackgroundMusic& music) {
    return "state="+std::to_string(static_cast<int>(music.status()))+
        " position="+std::to_string(music.positionMilliseconds())+
        " duration="+std::to_string(music.durationMilliseconds())+
        " loops="+std::to_string(music.loopCount())+
        " total="+std::to_string(playedMilliseconds(music))+
        " error="+music.error();
}
void trace(const char* phase,const br::BackgroundMusic& music,Clock::time_point started) {
    const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-started).count();
    std::cout<<"Music lifecycle "<<phase<<" wall_ms="<<elapsed<<' '<<describe(music)<<std::endl;
}
struct TemporaryMusic {
    std::filesystem::path path;
    TemporaryMusic() {
        const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count();
        path=std::filesystem::temp_directory_path()/std::filesystem::u8path(
            u8"Backrooms music \u0442\u0435\u0441\u0442 "+std::to_string(stamp)+".wav");
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
    auto previous=Clock::now();
    const auto deadline=previous+std::chrono::milliseconds(milliseconds);
    while(previous<deadline) {
        std::this_thread::sleep_until(std::min(deadline,previous+std::chrono::milliseconds(25)));
        const auto now=Clock::now();
        music.update(paused,muted,volume,std::chrono::duration<float>(now-previous).count());
        previous=now;
    }
}
template<class Predicate>
void waitForPlayback(br::BackgroundMusic& music,Predicate reached,const char* reason,int timeoutMilliseconds=8000) {
    auto previous=Clock::now();
    const auto deadline=previous+std::chrono::milliseconds(timeoutMilliseconds);
    auto previousPosition=playedMilliseconds(music);
    while(!reached()) {
        check(previous<deadline,std::string(reason)+" timed out: "+describe(music));
        std::this_thread::sleep_until(std::min(deadline,previous+std::chrono::milliseconds(25)));
        const auto now=Clock::now();
        music.update(false,false,.4f,std::chrono::duration<float>(now-previous).count());
        const auto position=playedMilliseconds(music);
        check(music.available(),std::string(reason)+" lost output: "+describe(music));
        check(position>=previousPosition,std::string(reason)+" rewound playback from "+
            std::to_string(previousPosition)+": "+describe(music));
        previousPosition=position;
        previous=now;
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
    const auto started=Clock::now();
    TemporaryMusic temporary;
    writeSilentWav(temporary.path);
    br::BackgroundMusic music;
    auto missingMp3=temporary.path;
    missingMp3.replace_extension(L".mp3");
    const bool loaded=music.load(missingMp3);
    check(loaded,"Unicode/spaced filename and WAV fallback load: "+music.error());
    check(music.available() && music.status()==br::MusicStatus::Ready,"load never autoplays");
    check(music.durationMilliseconds()>=1950 && music.durationMilliseconds()<=2050,"native duration is in milliseconds");
    trace("loaded",music,started);
    tick(music,100,true);
    check(music.positionMilliseconds()==0 && music.status()==br::MusicStatus::Ready,"menu does not start soundtrack");
    waitForPlayback(music,[&]{return playedMilliseconds(music)>=150;},"device advances during gameplay");
    check(music.status()==br::MusicStatus::Playing,"device is playing: "+describe(music));
    trace("playing",music,started);
    music.update(true,false,.4f,.025f);
    check(music.status()==br::MusicStatus::Paused,"pause menu pauses native playback");
    const auto pausedAt=playedMilliseconds(music);
    trace("paused",music,started);
    tick(music,350,true);
    check(playedMilliseconds(music)==pausedAt,"paused position is preserved: "+describe(music));
    music.update(false,false,.4f,0);
    check(playedMilliseconds(music)>=pausedAt,"resume must not rewind the track: "+describe(music));
    // Position wraps at a natural loop. Compare cumulative playback and wait
    // for an observed advance, rather than assuming sleep_for runs on time.
    waitForPlayback(music,[&]{return playedMilliseconds(music)>=pausedAt+150;},"resuming continues from same position");
    trace("resumed",music,started);
    music.update(false,true,.4f,.025f);
    check(music.status()==br::MusicStatus::Paused,"M-key mute suspends music");
    const auto mutedAt=playedMilliseconds(music);
    const auto mutedLoops=music.loopCount();
    tick(music,100,false,true);
    check(playedMilliseconds(music)==mutedAt,"mute preserves playback position: "+describe(music));
    trace("muted",music,started);
    music.update(false,false,.4f,0);
    check(playedMilliseconds(music)>=mutedAt,"unmute must not rewind the track: "+describe(music));
    waitForPlayback(music,[&]{return music.loopCount()>mutedLoops;},"track loops after natural end");
    check(music.available() && music.status()==br::MusicStatus::Playing,"loop retains playing output: "+describe(music));
    trace("looped",music,started);
    const auto nextBoundary=(static_cast<std::uint64_t>(music.loopCount())+1)*music.durationMilliseconds();
    waitForPlayback(music,[&]{return playedMilliseconds(music)>=nextBoundary-300;},"approach the next loop boundary");
    music.update(true,false,.4f,0);
    const auto boundaryPausedAt=playedMilliseconds(music);
    const auto boundaryLoops=music.loopCount();
    music.update(false,false,.4f,0);
    check(playedMilliseconds(music)>=boundaryPausedAt,"resume near loop boundary must not rewind: "+describe(music));
    waitForPlayback(music,[&]{return music.loopCount()>boundaryLoops;},"resume across a natural loop boundary");
    check(playedMilliseconds(music)>boundaryPausedAt,"loop boundary advances cumulative playback: "+describe(music));
    trace("resumed-across-loop",music,started);
    music.update(false,false,std::numeric_limits<float>::quiet_NaN(),.025f);
    check(music.status()==br::MusicStatus::Paused,"invalid volume fails silent");
    music.update(false,false,2,.025f);
    check(music.status()==br::MusicStatus::Playing,"volume clamps to valid range");
    music.stop();
    check(!music.available() && music.error().empty(),"stop closes device cleanly");
    std::error_code error;
    check(std::filesystem::remove(temporary.path,error),"device released the open audio file");
    trace("closed",music,started);
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
