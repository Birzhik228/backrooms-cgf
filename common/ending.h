#pragma once
#include "math.h"
#include <algorithm>
#include <cmath>

namespace br {
inline float endingEase(float a,float b,float time) {
    const float t=std::clamp((time-a)/(b-a),0.f,1.f);
    return t*t*(3.f-2.f*t);
}
inline constexpr float ENDING_DURATION=42;
struct EndingPose {
    Vec3 eye{0,1.62f,.65f},forward{0,0,-1};
    float elevatorOpen=1,exitOpen=0,cabinLight=1,lobbyLight=1;
    float fov=64,fade=0;
    bool surface=false;
};
inline EndingPose sampleEndingPose(float time) {
    const float t=std::isfinite(time)?std::clamp(time,0.f,ENDING_DURATION):0.f;
    EndingPose p;
    p.elevatorOpen=1-endingEase(.7f,3.f,t)+endingEase(10.3f,13.f,t);
    p.exitOpen=endingEase(20.4f,23.1f,t);
    p.surface=t>=5;
    const float lobbyWalk=endingEase(13.5f,20.f,t);
    const float outsideWalk=endingEase(23.2f,30.f,t);
    const float clearedDoor=endingEase(26.6f,30.f,t);
    const float lookAlongStreet=endingEase(30.f,38.5f,t);
    p.eye.z=.65f-8.35f*lobbyWalk-6.5f*outsideWalk;
    // Keep the camera centered until it has fully cleared the door jamb/leaf.
    p.eye.x=.12f*lobbyWalk+1.28f*clearedDoor;
    const float lobbyBob=(t>13.5f&&t<20.f)?std::sin((t-13.5f)*8)*.011f*std::sin(PI*lobbyWalk):0;
    const float outsideBob=(t>23.2f&&t<30.f)?std::sin((t-23.2f)*8)*.011f*std::sin(PI*outsideWalk):0;
    p.eye.y+=lobbyBob+outsideBob;
    const float motor=endingEase(3,3.5f,t)*(1-endingEase(9.4f,10,t));
    p.eye.x+=motor*.002f*std::sin(t*67);
    p.eye.y+=motor*.003f*std::sin(t*55);
    const float heading=.06f*clearedDoor+.56f*lookAlongStreet;
    p.forward=normalize(Vec3{std::sin(heading),-.015f-.025f*lobbyWalk+.10f*outsideWalk,-std::cos(heading)});
    p.cabinLight=1-.15f*motor;
    // The ordinary city is the real destination; the lobby does not dim at exit.
    p.lobbyLight=1;
    p.fov=64+2*lobbyWalk+5*outsideWalk;
    p.fade=std::max(1-endingEase(0,.7f,t),endingEase(39,ENDING_DURATION,t));
    return p;
}
class EndingSequence {
    double time_=0;
    bool running_=false;
public:
    static constexpr float DURATION=ENDING_DURATION;
    void start(float at=0) {time_=std::isfinite(at)?std::clamp(at,0.f,DURATION):0;running_=true;}
    void reset(){time_=0;running_=false;}
    void update(float dt,bool paused=false) {
        if(running_&&!paused&&std::isfinite(dt)&&dt>0)
            time_=std::min(double(DURATION),time_+std::min(double(dt),.25));
    }
    float time()const{return float(time_);}
    bool finished()const{return running_&&time_>=DURATION;}
    static const char* phaseName(float t) {
        return t<3?"Doors closing":t<10.3f?"Ascending":t<13.5f?"Surface":
               t<20.4f?"Lobby":t<23.2f?"Exit opening":t<30?"City street":
               t<39?"City life":t<DURATION?"Fade":"Complete";
    }
};
}
