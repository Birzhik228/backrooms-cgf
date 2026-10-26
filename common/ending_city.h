#pragma once
#include "math.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace br {
struct CityActorPose {
    Vec3 position{};
    // Rotation about Y, using local -Z as the model's forward direction.
    float yaw=0,gait=0,speed=0;
};
// Pure samples of the cinematic clock: pausing/replaying the film also freezes/
// resets every pedestrian limb and wheel. No AI, allocation or growing history.
struct EndingCity {
    static constexpr int pedestrianCount=12,carCount=6;
    static constexpr float streetHalfLength=90;
    static CityActorPose pedestrian(int index,float time) {
        const int id=normalizedIndex(index,pedestrianCount),side=id/6,slot=id%6;
        const float direction=slot%2==0?1.f:-1.f;
        const float speed=1.05f+.06f*(slot%4);
        constexpr std::array<float,6> nearAt32={-18,-7,-3,4,15,24};
        constexpr std::array<float,6> farAt32={-20,-12,-4,4,12,20};
        const double t=clock(time),distance=speed*t;
        const double anchor=side==0?nearAt32[slot]:farAt32[slot];
        const float z=side==0?(slot%2?-16.2f:-15.6f):(slot%2?-29.6f:-28.8f);
        return {{wrap(anchor+direction*speed*(t-32)),0,z},
                direction>0?-PI*.5f:PI*.5f,phase(distance/1.30*2*PI+slot*.71),speed};
    }
    static CityActorPose car(int index,float time) {
        const int id=normalizedIndex(index,carCount),lane=id/3,slot=id%3;
        const float direction=lane==0?1.f:-1.f,speed=lane==0?6.f:5.8f;
        constexpr std::array<float,3> nearAt32={-38,-6,26},farAt32={-26,6,38};
        const double t=clock(time),anchor=lane==0?nearAt32[slot]:farAt32[slot];
        return {{wrap(anchor+direction*speed*(t-32)),-.15f,lane==0?-20.f:-24.f},
                direction>0?-PI*.5f:PI*.5f,phase(t*speed/.32),speed};
    }
private:
    static int normalizedIndex(int index,int count){return (index%count+count)%count;}
    static double clock(float time){return std::isfinite(time)?std::max(0.0,double(time)):0.0;}
    static float wrap(double x) {
        const double span=2*streetHalfLength;
        double shifted=std::fmod(x+streetHalfLength,span);if(shifted<0)shifted+=span;
        return float(shifted-streetHalfLength);
    }
    static float phase(double radians) {
        double wrapped=std::fmod(radians,double(2*PI));if(wrapped<0)wrapped+=2*PI;
        return float(wrapped);
    }
};
}
