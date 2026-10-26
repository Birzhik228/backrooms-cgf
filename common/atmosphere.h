#pragma once

#include "audio.h"
#include "../world.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>

namespace br {

struct AtmosphereContext {
    AudioPosition listener{},threat{};
    bool paused=false,alive=true,hidden=false;
    bool threatActive=false,threatVisible=false,threatDanger=false;
};

struct AtmosphereCue {
    SoundKind kind=SoundKind::VentRattle;
    AudioPosition position{};
    float gain=1,transmission=1;
};

// Sparse Level 0 foley. This owns only sound timing: it never moves/spawns an
// actor or sends AI hearing events. All positions remain in world coordinates.
class Level0Atmosphere {
public:
    explicit Level0Atmosphere(std::uint64_t seed=1) { reset(seed); }
    void reset(std::uint64_t seed) {
        random_=seed^0x9e3779b97f4a7c15ULL;
        if(!random_)random_=1;
        ambient_=between(9,15);steps_=between(28,42);rustle_=between(12,20);
        gap_=recovery_=0;lastAmbient_=-1;
    }

    std::optional<AtmosphereCue> update(const World& world,const AtmosphereContext& state,float dt) {
        if(world.level()!=Level::Zero || state.paused || !state.alive ||
           !std::isfinite(dt) || dt<=0 || !finite(state.listener))return {};
        dt=std::min(dt,.1f); // A stall/teleport cannot release a burst of events.
        const double threatDistance=state.threatActive&&finite(state.threat)?
            std::hypot(state.threat.x-state.listener.x,state.threat.z-state.listener.z):1e9;
        if(state.hidden || state.threatDanger || state.threatVisible || threatDistance<10) {
            recovery_=8;
            return {};
        }
        if(recovery_>0) { recovery_=std::max(0.f,recovery_-dt);return {}; }
        ambient_-=dt;steps_-=dt;rustle_-=dt;gap_-=dt;
        if(gap_>0)return {};

        // A presence hint is emitted only from the real, occluded creature.
        // The caller suppresses Chase/Search so this never masks actionable cues.
        if(rustle_<=0) {
            rustle_=4;
            if(state.threatActive && threatDistance>=10 && threatDistance<=22 &&
               !world.blocked(state.threat.x,state.threat.z,.18)) {
                const float transmission=world.soundTransmission(state.listener.x,state.listener.z,state.threat.x,state.threat.z);
                if(transmission>=.3f && transmission<=.5f) {
                    rustle_=between(35,60);
                    gap_=between(8,12);
                    return AtmosphereCue{SoundKind::ThreatRustle,{state.threat.x,1.35,state.threat.z},.7f,transmission};
                }
            }
        }
        if(steps_<=0) {
            steps_=between(48,78);
            auto cue=place(world,state.listener,SoundKind::DistantFootsteps,7,16,true,.8f);
            if(cue){gap_=between(8,12);return cue;}
            // Retry a failed placement later, never raycast every frame.
            steps_=7;
        }
        if(ambient_<=0) {
            ambient_=between(16,28);
            int choice=int(unit()*3);
            if(choice==lastAmbient_)choice=(choice+1+int(unit()*2))%3;
            const SoundKind kinds[]={SoundKind::VentRattle,SoundKind::AmbientCreak,SoundKind::WaterDrip};
            auto cue=place(world,state.listener,kinds[choice],4,10,false,choice==0?.6f:.48f);
            if(cue){lastAmbient_=choice;gap_=between(7,10);return cue;}
            ambient_=5;
        }
        return {};
    }

private:
    std::uint64_t random_=1;
    float ambient_=0,steps_=0,rustle_=0,gap_=0,recovery_=0;
    int lastAmbient_=-1;
    static bool finite(AudioPosition point) {
        return std::isfinite(point.x)&&std::isfinite(point.y)&&std::isfinite(point.z);
    }
    double unit() {
        random_^=random_>>12;random_^=random_<<25;random_^=random_>>27;
        return double((random_*2685821657736338717ULL)>>11)*(1.0/9007199254740992.0);
    }
    float between(float low,float high){return low+float(unit())*(high-low);}
    std::optional<AtmosphereCue> place(const World& world,AudioPosition listener,SoundKind kind,
                                      double minimum,double maximum,bool behindWall,float gain) {
        constexpr double tau=6.28318530717958647692;
        for(int attempt=0;attempt<16;++attempt) {
            const double angle=unit()*tau,distance=minimum+unit()*(maximum-minimum);
            const AudioPosition source{listener.x+std::cos(angle)*distance,
                behindWall?.2:2.65,listener.z+std::sin(angle)*distance};
            // blocked() rejects unloaded chunks as well as walls/closed doors.
            if(world.blocked(source.x,source.z,.2))continue;
            const float transmission=world.soundTransmission(listener.x,listener.z,source.x,source.z);
            if(transmission<.12f || (behindWall&&transmission>.5f))continue;
            return AtmosphereCue{kind,source,gain,transmission};
        }
        return {};
    }
};
}
