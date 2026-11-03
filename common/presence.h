#pragma once

#include "../world.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace br {

struct PresenceContext {
    WorldPoint listener{},threat{};
    float forwardX=0,forwardZ=-1;
    bool paused=false,alive=true,hidden=false;
    bool threatActive=false,threatWandering=false;
};

struct DoorRattleCue {
    std::uint64_t doorId=0;
    WorldPoint position{};
};

// A visual hint from the actual wandering creature. This reads loaded geometry
// only: it never opens a leaf, moves an actor, alters collision or sends AI noise.
class Level0Presence {
public:
    static constexpr float DURATION=.85f;
    static constexpr std::size_t MAX_CANDIDATES=8;

    explicit Level0Presence(std::uint64_t seed=1) { reset(seed); }
    void reset(std::uint64_t seed) {
        random_=seed^0x637265616b646f6fULL;
        if(!random_)random_=1;
        cooldown_=between(12,20);scan_=phase_=0;
        active_.reset();owner_={};candidateChecks_=0;
    }

    std::optional<std::uint64_t> doorId() const { return active_; }
    bool active() const { return bool(active_); }
    unsigned candidateChecks() const { return candidateChecks_; }
    float handleAngle() const {
        if(!active_)return 0;
        // Two smooth downward pulls, with a still interval and a settled tail.
        const auto pulse=[](float t,float start,float length) {
            if(t<=start || t>=start+length)return 0.f;
            const float s=std::sin((t-start)/length*3.14159265358979323846f);
            return s*s;
        };
        return -.30f*(pulse(phase_,0,.30f)+.8f*pulse(phase_,.40f,.30f));
    }

    std::optional<DoorRattleCue> update(const World& world,const PresenceContext& state,float dt) {
        candidateChecks_=0;
        if(world.level()!=Level::Zero || !state.alive || state.hidden ||
           !state.threatActive || !state.threatWandering ||
           !finite(state.listener) || !finite(state.threat) ||
           !std::isfinite(state.forwardX) || !std::isfinite(state.forwardZ)) {
            active_.reset();phase_=0;
            return {};
        }
        if(state.paused || !std::isfinite(dt) || dt<=0)return {};
        dt=std::min(dt,.1f); // A stalled frame cannot finish a cooldown at once.
        cooldown_=std::max(0.f,cooldown_-dt);scan_=std::max(0.f,scan_-dt);

        if(active_) {
            const auto chunk=world.chunks().find(owner_);
            const DoorInfo* door=nullptr;
            if(chunk!=world.chunks().end())for(const auto& item:chunk->second.doors)
                if(item.id==*active_){door=&item;break;}
            // During the brief animation, inexpensive range/state checks are
            // sufficient. Geometry rays run only when selecting a new event.
            if(!door || !eligible(*door,state)) { active_.reset();phase_=0;return {}; }
            phase_+=dt;
            if(phase_>=DURATION){active_.reset();phase_=0;}
            return {};
        }
        if(cooldown_>0 || scan_>0)return {};
        scan_=1;

        struct Candidate { DoorInfo door{};ChunkCoord owner{};double distance=0; };
        std::array<Candidate,MAX_CANDIDATES> candidates{};
        std::size_t count=0;
        for(const auto& [coord,chunk]:world.chunks())for(const auto& door:chunk.doors) {
            if(!eligible(door,state))continue;
            const double distance=std::hypot(door.x-state.listener.x,door.z-state.listener.z);
            const Candidate item{door,coord,distance};
            if(count<MAX_CANDIDATES)candidates[count++]=item;
            else {
                auto farthest=std::max_element(candidates.begin(),candidates.end(),
                    [](const Candidate& a,const Candidate& b){return a.distance<b.distance;});
                if(distance<farthest->distance)*farthest=item;
            }
        }
        std::sort(candidates.begin(),candidates.begin()+count,[](const Candidate& a,const Candidate& b){
            return a.distance!=b.distance?a.distance<b.distance:a.door.id<b.door.id;
        });
        for(std::size_t i=0;i<count;++i) {
            ++candidateChecks_;
            const auto& door=candidates[i].door;
            const double playerSide=door.east?state.listener.x-door.x:state.listener.z-door.z;
            const double side=playerSide>0?1:-1;
            const WorldPoint nearApproach{door.x+(door.east?.35*side:0),door.z+(door.east?0:.35*side)};
            const WorldPoint farApproach{door.x-(door.east?.35*side:0),door.z-(door.east?0:.35*side)};
            if(world.blocked(state.threat.x,state.threat.z,.26) ||
               !clear(world,state.listener,nearApproach) || !clear(world,state.threat,farApproach) ||
               world.soundTransmission(state.listener.x,state.listener.z,state.threat.x,state.threat.z)>.5f)continue;
            active_=door.id;owner_=candidates[i].owner;phase_=0;
            cooldown_=between(40,65);
            // The ordinary handle lies 0.865 m along the closed door's plane.
            return DoorRattleCue{door.id,{door.x+(door.east?0:.865),door.z+(door.east?.865:0)}};
        }
        return {};
    }

private:
    std::uint64_t random_=1;
    float cooldown_=0,scan_=0,phase_=0;
    std::optional<std::uint64_t> active_;
    ChunkCoord owner_{};
    unsigned candidateChecks_=0;

    static bool finite(WorldPoint p){return std::isfinite(p.x)&&std::isfinite(p.z);}
    float between(float low,float high) {
        random_^=random_>>12;random_^=random_<<25;random_^=random_>>27;
        const double unit=double((random_*2685821657736338717ULL)>>11)*(1.0/9007199254740992.0);
        return low+float(unit)*(high-low);
    }
    static bool eligible(const DoorInfo& door,const PresenceContext& state) {
        if(door.isExit || door.open || door.openness>.001f)return false;
        const double dx=door.x-state.listener.x,dz=door.z-state.listener.z;
        const double distance=std::hypot(dx,dz),forward=std::hypot(state.forwardX,state.forwardZ);
        if(distance<5 || distance>12 || forward<.01 ||
           (dx*state.forwardX+dz*state.forwardZ)<.8*distance*forward ||
           std::hypot(state.threat.x-state.listener.x,state.threat.z-state.listener.z)<7 ||
           std::hypot(state.threat.x-door.x,state.threat.z-door.z)>4.5)return false;
        const double playerSide=door.east?state.listener.x-door.x:state.listener.z-door.z;
        const double threatSide=door.east?state.threat.x-door.x:state.threat.z-door.z;
        const double lateral=door.east?state.threat.z-door.z:state.threat.x-door.x;
        return playerSide*threatSide<0 && std::abs(threatSide)>=.3 && std::abs(lateral)<=2.5;
    }
    static bool clear(const World& world,WorldPoint from,WorldPoint to) {
        const int samples=std::max(1,int(std::ceil(std::hypot(to.x-from.x,to.z-from.z)/.20)));
        for(int i=0;i<=samples;++i) {
            const double t=double(i)/samples;
            if(world.blocked(from.x+(to.x-from.x)*t,from.z+(to.z-from.z)*t,.06))return false;
        }
        return true;
    }
};
}
