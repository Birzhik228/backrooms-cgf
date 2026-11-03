#pragma once
#include "audio.h"
#include "partygoer.h"
#include <array>
#include <cmath>
#include <optional>
#include <vector>

namespace br {
struct FunAudioContext {
    AudioPosition listener{};
    bool paused=false,alive=true;
};
struct FunAudioCue {
    SoundKind kind=SoundKind::PartyFootstep;
    AudioPosition position{};
    float gain=1,transmission=1;
    int actor=-1;
};

// Owns sound cadence only. Actor position, visibility and AI remain authoritative.
class FunAudioDirector {
public:
    explicit FunAudioDirector(std::uint64_t seed=1){reset(seed);}
    void reset(std::uint64_t seed) {
        random_=seed^0x5041525459415544ULL;if(!random_)random_=1;
        previous_={};tracked_={};distance_={};movingMask_=0;
        ambient_=between(9,14);recovery_=0;events_=candidateChecks_=0;quiet_=false;
    }
    std::uint8_t movingMask()const{return movingMask_;}
    bool quiet()const{return quiet_;}
    std::size_t candidateChecks()const{return candidateChecks_;}
    std::vector<FunAudioCue> update(const World& world,const Partygoers& party,
                                  const FunAudioContext& context,float dt) {
        std::vector<FunAudioCue> cues;
        movingMask_=0;quiet_=true;
        if(!world.isFun()||context.paused||!context.alive||!finite(context.listener)||
           !std::isfinite(dt)||dt<=0){tracked_={};distance_={};return cues;}
        dt=std::min(dt,.1f);
        bool danger=party.alarmRemaining>0;
        for(int i=0;i<Partygoers::COUNT;++i) {
            const auto& actor=party.actors[i];
            const AudioPosition position{actor.x,.22,actor.z};
            if(!actor.active||!finite(position)){tracked_[i]=false;distance_[i]=0;continue;}
            const double range=std::hypot(actor.x-context.listener.x,actor.z-context.listener.z);
            danger|=range<8;
            const double moved=tracked_[i]?std::hypot(actor.x-previous_[i].x,actor.z-previous_[i].z):0;
            previous_[i]={actor.x,actor.z};
            const bool wasTracked=tracked_[i];tracked_[i]=true;
            // Spawning, teleporting, gazing, turning in place and blocked pursuit
            // cannot create a step; relocation is never converted to distance.
            if(!wasTracked||actor.observed||moved<.00001||moved>1.) {
                if(!wasTracked||actor.observed||moved>1.)distance_[i]=0;
                continue;
            }
            movingMask_|=std::uint8_t(1u<<i);
            distance_[i]+=moved;
            if(distance_[i]<1.65)continue;
            distance_[i]=std::fmod(distance_[i],1.65);
            if(range>26||world.blocked(actor.x,actor.z,.12))continue;
            const float transmission=world.soundTransmission(context.listener.x,context.listener.z,actor.x,actor.z);
            if(transmission<.12f)continue;
            cues.push_back({SoundKind::PartyFootstep,position,.82f,transmission,i});
        }
        quiet_=danger;
        if(danger){recovery_=5;return cues;}
        if(recovery_>0){recovery_=std::max(0.f,recovery_-dt);return cues;}
        ambient_-=dt;
        if(ambient_>0)return cues;
        // A bounded 7x7 cell scan runs only when one sparse cue is due.
        const bool melody=events_%3==1;
        const auto cue=place(world,context.listener,melody);
        if(cue){cues.push_back(*cue);++events_;ambient_=between(23,38);}
        else {
            // A room without an audible toy box must not stall balloon foley
            // indefinitely. Retry the next decoration type after a quiet gap.
            if(melody)++events_;
            ambient_=5;
        }
        return cues;
    }
private:
    std::array<WorldPoint,Partygoers::COUNT> previous_{};
    std::array<bool,Partygoers::COUNT> tracked_{};
    std::array<double,Partygoers::COUNT> distance_{};
    std::uint64_t random_=1;
    std::uint8_t movingMask_=0;
    float ambient_=0,recovery_=0;
    std::size_t events_=0,candidateChecks_=0;
    bool quiet_=false;
    static bool finite(AudioPosition p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
    double unit(){random_^=random_>>12;random_^=random_<<25;random_^=random_>>27;
        return double((random_*2685821657736338717ULL)>>11)/9007199254740992.;}
    float between(float a,float b){return a+float(unit())*(b-a);}
    std::optional<FunAudioCue> place(const World& world,AudioPosition listener,bool melody) {
        const auto cx=std::int64_t(std::floor(listener.x/CELL)),cz=std::int64_t(std::floor(listener.z/CELL));
        const auto exit=exitCell(world.seed(),Level::Fun);
        const int offset=int(unit()*49);
        for(int i=0;i<49;++i) {
            ++candidateChecks_;
            const int index=(offset+i*17)%49;
            const auto gx=cx+index%7-3,gz=cz+index/7-3;
            if(gx==exit.x&&gz==exit.z)continue;
            const auto h=hashCell(gx,gz,world.seed()^0x46554e);
            AudioPosition position;
            if(melody) {
                // Matches the physical toy box on a seeded main table in
                // fun_geometry.h; a table without that prop stays silent.
                if((h>>36)%7!=0 || (h%2!=0 && (gx!=0||gz!=0)))continue;
                const double jitter=double((h>>8)%201)*.001-.1;
                position={gx*CELL+1.85+jitter,1.,gz*CELL+1.22};
            } else {
                if((h>>16)%3==0 && (gx!=0||gz!=0))continue;
                position={gx*CELL+6.1,2.1,gz*CELL+1.45};
            }
            const double range=std::hypot(position.x-listener.x,position.z-listener.z);
            if(range<4||range>18||!world.chunks().count(chunkAt(position.x,position.z)))continue;
            const float transmission=Partygoers::clearSight(world,listener.x,listener.y,listener.z,
                    position.x,position.y,position.z)?1.f:
                std::min(.38f,world.soundTransmission(listener.x,listener.z,position.x,position.z));
            if(transmission<.12f)continue;
            return FunAudioCue{melody?SoundKind::PartyMelody:SoundKind::BalloonRub,position,
                melody?.72f:.65f,transmission,-1};
        }
        return {};
    }
};
}
