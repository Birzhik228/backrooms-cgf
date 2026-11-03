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
    float headYaw=0,brake=0;
};
// Pure samples of the cinematic clock: pausing/replaying the film also freezes/
// resets every pedestrian limb and wheel. No AI, allocation or growing history.
struct EndingCity {
    static constexpr int pedestrianCount=12,carCount=6,crossingPedestrian=11;
    static constexpr float streetHalfLength=90,crossingX=17.55f;
    static constexpr double pedestrianStride=1.08;
    static double phaseOffset(int index){return .71*(normalizedIndex(index,pedestrianCount)%6);}
    static double gaitCycles(int index,float time) {
        const int id=normalizedIndex(index,pedestrianCount),slot=id%6;
        const double speed=id==crossingPedestrian?1.35:double(1.05f+.06f*(slot%4));
        return clock(time)*speed/pedestrianStride+phaseOffset(id)/(2*double(PI));
    }
    // Physical sidewalk, road and raised curb tops for the renderer's foot IK.
    static float groundHeight(float x,float z) {
        if(std::abs(x)>110)return 0;
        if((z>=-27.04f&&z<=-26.84f)||(z>=-17.16f&&z<=-16.94f))return .035f;
        return z>=-27.f&&z<=-17.f?-.15f:0.f;
    }
    static bool crossingActive(float time){const double t=clock(time);return t>=31&&t<39.25;}
    static bool trafficAmber(float time){const double t=clock(time);return t>=29&&t<31;}
    static CityActorPose pedestrian(int index,float time) {
        const int id=normalizedIndex(index,pedestrianCount),side=id/6,slot=id%6;
        const double t=clock(time);
        if(id==crossingPedestrian)return crossingWalker(t);
        const float direction=slot%2==0?1.f:-1.f;
        const float speed=1.05f+.06f*(slot%4);
        // Leave the late camera's foreground clear enough to read the crossing.
        // Opposite walking lanes have room for shoulders and a carried bag.
        constexpr std::array<float,6> nearAt32={-24,-9,6,18,33,44};
        constexpr std::array<float,6> farAt32={-20,-12,-4,4,1,20};
        const double anchor=side==0?nearAt32[slot]:farAt32[slot];
        const float z=side==0?(slot%2?-16.4f:-15.5f):(slot%2?-29.7f:-28.8f);
        CityActorPose p{{wrap(anchor+direction*speed*(t-32)),0,z},
                       direction>0?-PI*.5f:PI*.5f,phase(gaitCycles(id,time)*2*PI),speed};
        if(id==3||id==8) {
            // An ordinary glance toward the doorway, then eyes back on the walk.
            const float target=std::atan2(p.position.x-1.4f,p.position.z+14.2f);
            const float relative=std::remainder(target-p.yaw,2*PI);
            p.headYaw=std::clamp(relative,-.9f,.9f)*pulse(t,31,32.4,35.8,37.2);
        }
        return p;
    }
    static CityActorPose car(int index,float time) {
        const int id=normalizedIndex(index,carCount),lane=id/3,slot=id%3;
        const float direction=lane==0?1.f:-1.f,cruise=lane==0?6.f:5.8f;
        // At the stop, front bumpers stay clear of the zebra. Applying the same
        // travel function to each lane preserves headway even while braking.
        constexpr std::array<float,3> nearAtStop={-51.5f,-19.5f,12.5f},farAtStop={22.5f,54.5f,86.5f};
        const double t=clock(time),anchor=lane==0?nearAtStop[slot]:farAtStop[slot];
        const double distance=trafficTravel(t)*cruise;
        CityActorPose p{{wrap(anchor+direction*distance),-.15f,lane==0?-20.f:-24.f},
                       direction>0?-PI*.5f:PI*.5f,
                       phase((distance-trafficTravel(0)*cruise)/.32),cruise*trafficSpeed(t)};
        p.brake=pulse(t,29,29.2,39.25,39.65);
        return p;
    }
private:
    static int normalizedIndex(int index,int count){return (index%count+count)%count;}
    static double clock(float time){return std::isfinite(time)?std::max(0.0,double(time)):0.0;}
    static double smooth(double u){u=std::clamp(u,0.0,1.0);return u*u*(3-2*u);}
    static float pulse(double t,double a,double b,double c,double d) {
        return float(smooth((t-a)/(b-a))*(1-smooth((t-c)/(d-c))));
    }
    static float trafficSpeed(double t) {
        if(t<29)return 1;
        if(t<31)return float(1-smooth((t-29)/2));
        if(t<39.25)return 0;
        return float(smooth((t-39.25)/2));
    }
    // Exact integral of the smooth speed envelope, in seconds of cruise travel.
    // Anchored at the stop, so wheel rotation and translation cannot disagree.
    static double trafficTravel(double t) {
        if(t<=29)return t-30;
        if(t<31){const double u=(t-29)/2;return -1+2*(u-u*u*u+.5*u*u*u*u);}
        if(t<=39.25)return 0;
        if(t<41.25){const double u=(t-39.25)/2;return 2*(u*u*u-.5*u*u*u*u);}
        return t-40.25;
    }
    static CityActorPose crossingWalker(double t) {
        constexpr double speed=1.35,entryRadius=1.2,exitRadius=.9;
        const double entryAt=31-entryRadius*double(PI)*.5/speed;
        const double turnAt=31+11.2/speed,exitAt=turnAt+exitRadius*double(PI)*.5/speed;
        CityActorPose p;p.speed=float(speed);p.gait=phase((t*speed/pedestrianStride+phaseOffset(crossingPedestrian)/(2*double(PI)))*2*PI);
        p.position.y=0;
        if(t<entryAt) {
            p.position={wrap(crossingX-entryRadius+speed*(t-entryAt)),0,-28.8f};p.yaw=-PI*.5f;
        } else if(t<31) {
            const double a=speed*(t-entryAt)/entryRadius;
            p.position={float(crossingX-entryRadius+entryRadius*std::sin(a)),0,float(-28.8+entryRadius*(1-std::cos(a)))};
            p.yaw=float(-PI*.5-a);
        } else if(t<turnAt) {
            p.position={crossingX,0,float(-27.6+speed*(t-31))};p.yaw=-PI;
        } else if(t<exitAt) {
            const double a=speed*(t-turnAt)/exitRadius;
            p.position={float(crossingX+exitRadius*(1-std::cos(a))),0,float(-16.4+exitRadius*std::sin(a))};
            p.yaw=float(-PI+a);
        } else {
            p.position={wrap(crossingX+exitRadius+speed*(t-exitAt)),0,-15.5f};p.yaw=-PI*.5f;
        }
        // The body follows the curb step gradually; the renderer samples the
        // physical floor separately for each planted foot. It never floats at
        // sidewalk height while crossing the recessed road.
        const double z=p.position.z;
        const double road=smooth((z+27.05)/.45)*(1-smooth((z+17.45)/.45));
        p.position.y=float(-.15*road+.035*pulse(z,-27.4,-27.1,-26.9,-26.6)+
                           .035*pulse(z,-17.5,-17.2,-16.9,-16.6));
        // Brief look across the road before stepping off the pavement.
        p.headYaw=.42f*pulse(t,29.3,29.8,30.4,31.1);
        return p;
    }
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
