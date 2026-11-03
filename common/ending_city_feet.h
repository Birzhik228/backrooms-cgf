#pragma once
#include "ending_city.h"

namespace br {
struct CityFootPose {
    Vec3 position{}; // World-space ankle, including sole thickness.
    float yaw=0,phase=0,supportHeight=0;
    bool planted=false;
};
struct CityFeetPose {
    std::array<CityFootPose,2> feet{};
    float pelvisHeight=0; // Ground-height support, excluding body/hip dimensions.
};
// Contact points are sampled once mathematically per step, rather than resampled
// from a sliding sole. Pure time sampling keeps pause, seek and replay identical.
struct EndingCityFeet {
    static int actorIndex(int index){return (index%EndingCity::pedestrianCount+EndingCity::pedestrianCount)%EndingCity::pedestrianCount;}
    static float height(int index){return .94f+.03f*(actorIndex(index)%6);}
    static float width(int index){return .92f+.045f*(actorIndex(index)%5);}
    static float smooth(float a,float b,float x) {
        const float u=std::clamp((x-a)/(b-a),0.f,1.f);return u*u*(3-2*u);
    }
    static float angle(float a){return std::atan2(std::sin(a),std::cos(a));}
    static float soleGround(Vec3 ankle,float yaw,float breadth) {
        float floor=-.15f;
        for(float x:{-.08f*breadth,0.f,.08f*breadth})for(float z:{-.21f,0.f,.095f})
            floor=std::max(floor,EndingCity::groundHeight(ankle.x+x*std::cos(yaw)+z*std::sin(yaw),
                                                        ankle.z-x*std::sin(yaw)+z*std::cos(yaw)));
        return floor;
    }
    static CityFeetPose sample(int index,float time) {
        const auto body=EndingCity::pedestrian(index,time);
        const float tall=height(index),wide=width(index),speed=body.speed;
        CityFeetPose result;
        if(speed<.001f)return result;
        const double period=1.0/(EndingCity::gaitCycles(index,1)-EndingCity::gaitCycles(index,0));
        const double cycles=EndingCity::gaitCycles(index,time);
        const double offset=EndingCity::phaseOffset(index)/(2*double(PI));
        for(int foot=0;foot<2;++foot) {
            const int side=foot?1:-1;const double stagger=foot?.5:0;
            const double cycle=cycles+stagger,step=std::floor(cycle);
            const float phase=float(cycle-step);
            const double contactTime=(step-offset-stagger)*period;
            auto contact=[&](double at) {
                const auto root=EndingCity::pedestrian(index,float(at));
                CityFootPose pose;pose.yaw=root.yaw;
                const float x=side*.105f*wide,z=-.27f;
                pose.position={root.position.x+x*std::cos(root.yaw)+z*std::sin(root.yaw),0,
                               root.position.z-x*std::sin(root.yaw)+z*std::cos(root.yaw)};
                // Unwrap an offscreen endpoint around the current actor before
                // interpolation, so no swing stretches across the whole street.
                const float span=2*EndingCity::streetHalfLength;
                pose.position.x-=std::round((pose.position.x-body.position.x)/span)*span;
                pose.supportHeight=soleGround(pose.position,pose.yaw,wide);
                pose.position.y=pose.supportHeight+.056f*tall;
                return pose;
            };
            const auto from=contact(contactTime),to=contact(contactTime+period);
            auto& pose=result.feet[foot];pose=from;pose.phase=phase;pose.planted=phase<.5f;
            if(!pose.planted) {
                const float u=(phase-.5f)*2;
                float peak=std::max(from.supportHeight,to.supportHeight);
                const float lowZ=std::min(from.position.z,to.position.z)-.24f,highZ=std::max(from.position.z,to.position.z)+.24f;
                if((lowZ<=-26.84f&&highZ>=-27.04f)||(lowZ<=-16.94f&&highZ>=-17.16f))peak=std::max(peak,.035f);
                const bool stepUp=std::abs(from.supportHeight-to.supportHeight)>.001f||peak>from.supportHeight+.001f;
                // A kerb step lifts before translating and settles after the
                // sole clears the lip. Each segment has zero endpoint velocity.
                const float travel=stepUp?smooth(.18f,.82f,u):smooth(0,1,u);
                pose.position=from.position+(to.position-from.position)*travel;
                pose.yaw=from.yaw+angle(to.yaw-from.yaw)*travel;
                pose.supportHeight=from.supportHeight+(to.supportHeight-from.supportHeight)*travel;
                if(stepUp) {
                    const float top=peak+.10f*tall;
                    pose.position.y=(u<.18f?from.supportHeight+(top-from.supportHeight)*smooth(0,.18f,u):
                                     u>.82f?top+(to.supportHeight-top)*smooth(.82f,1,u):top)+.056f*tall;
                }else pose.position.y+=std::sin(PI*u)*std::sin(PI*u)*.085f*tall;
            }
        }
        // Shift load across the two planted support planes around contact.
        // Airborne ankle lift never raises or drops the pelvis.
        const float weight=smooth(-.5f,.5f,std::sin(result.feet[0].phase*2*PI));
        result.pelvisHeight=result.feet[1].supportHeight+(result.feet[0].supportHeight-result.feet[1].supportHeight)*weight;
        return result;
    }
};
}
