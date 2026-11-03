#pragma once
#include "audio.h"
#include "darkness.h"
#include <array>
#include <cmath>
#include <optional>
#include <vector>

namespace br {
struct ParkingAudioContext {
    AudioPosition listener{};
    bool paused=false,alive=true;
};
struct ParkingAudioCue {
    SoundKind kind=SoundKind::PipeTick;
    AudioPosition position{};
    float gain=1,transmission=1;
    int circuit=-1;
};

// Industrial foley owns no enemies or light state. Sources follow actual props;
// restoring power is observed once, never inferred from the player's location.
class ParkingAudioDirector {
public:
    explicit ParkingAudioDirector(std::uint64_t seed=1){reset(seed);}
    void reset(std::uint64_t seed) {
        random_=seed^0x5041524b415544ULL;if(!random_)random_=1;
        powered_={};initialized_=false;quiet_=false;recovery_=0;
        ambient_=between(8,13);nextKind_=0;candidateChecks_=0;
    }
    bool quiet()const{return quiet_;}
    std::size_t candidateChecks()const{return candidateChecks_;}
    std::vector<ParkingAudioCue> update(const World& world,const Darkness& darkness,
                                      const ParkingAudioContext& context,float dt) {
        std::vector<ParkingAudioCue> cues;
        quiet_=true;
        if(!world.isParking()){initialized_=false;return cues;}
        // R retains powered sectors. A new director snapshots that state
        // silently, including when the first frame is a paused guide/menu.
        if(!initialized_){for(int i=0;i<3;++i)powered_[i]=world.breakerPowered(i);initialized_=true;}
        if(context.paused||!context.alive||!finite(context.listener)||!std::isfinite(dt)||dt<=0)return cues;
        dt=std::min(dt,.1f);
        for(int i=0;i<3;++i) {
            const bool on=world.breakerPowered(i);
            if(on&&!powered_[i]) {
                const auto panel=world.breakerLocation(i);
                const AudioPosition source{panel.x,1.25,panel.z};
                if(std::hypot(source.x-context.listener.x,source.z-context.listener.z)<=48)
                    cues.push_back({SoundKind::PowerRelay,source,.9f,transmission(world,context.listener,source),i});
                ambient_=std::max(ambient_,4.f);
            }
            powered_[i]=on;
        }
        bool danger=false;
        for(const auto& eye:darkness.eyes)if(eye.active&&std::isfinite(eye.x)&&std::isfinite(eye.z)&&
            std::hypot(eye.x-context.listener.x,eye.z-context.listener.z)<=8)danger=true;
        quiet_=danger;
        if(danger){recovery_=5;return cues;}
        if(recovery_>0){recovery_=std::max(0.f,recovery_-dt);return cues;}
        ambient_-=dt;
        if(ambient_>0)return cues;
        const auto cue=place(world,context.listener,nextKind_);
        nextKind_=(nextKind_+1)%3;
        if(cue){cues.push_back(*cue);ambient_=between(17,29);}
        else ambient_=5; // A room lacking a vent/leak cannot stall the schedule.
        return cues;
    }
private:
    std::array<bool,3> powered_{};
    bool initialized_=false,quiet_=false;
    std::uint64_t random_=1;
    float ambient_=0,recovery_=0;
    int nextKind_=0;
    std::size_t candidateChecks_=0;
    static bool finite(AudioPosition p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
    double unit(){random_^=random_>>12;random_^=random_<<25;random_^=random_>>27;
        return double((random_*2685821657736338717ULL)>>11)/9007199254740992.;}
    float between(float a,float b){return a+float(unit())*(b-a);}
    static float transmission(const World& world,AudioPosition a,AudioPosition b) {
        return Partygoers::clearSight(world,a.x,a.y,a.z,b.x,b.y,b.z)?1.f:
            std::min(.38f,world.soundTransmission(a.x,a.z,b.x,b.z));
    }
    std::optional<ParkingAudioCue> place(const World& world,AudioPosition listener,int kind) {
        const auto cx=std::int64_t(std::floor(listener.x/CELL)),cz=std::int64_t(std::floor(listener.z/CELL));
        const int offset=int(unit()*49);
        for(int attempt=0;attempt<49;++attempt) {
            ++candidateChecks_;
            const int cell=(offset+attempt*17)%49;
            const auto gx=cx+cell%7-3,gz=cz+cell/7-3;
            const auto h=hashCell(gx,gz,world.seed()^0x5041524b494e47ULL);
            bool landmark=ChunkCoord{gx,gz}==parkingExitCell(world.seed());
            for(int i=0;i<3;++i)landmark|=ChunkCoord{gx,gz}==parkingBreakerCell(world.seed(),i);
            AudioPosition position;SoundKind sound=SoundKind::PipeTick;float gain=.62f;
            if(kind==1) {
                // Matches the drip and patch under the existing west pipe.
                if(landmark||(h>>40)%11!=0||h%11==0||(gx==0&&gz==0))continue;
                position={gx*CELL+.85,2.70,gz*CELL+4.05};sound=SoundKind::WaterDrip;gain=.5f;
            } else if(kind==2) {
                // A powerless grille is silent; restored sectors have a motor.
                if(landmark||(h>>32)%7!=0||!world.breakerPowered(parkingSector(gx,gz)))continue;
                position={gx*CELL+5.85,3.175,gz*CELL+3.0};sound=SoundKind::ParkingVent;gain=.64f;
            } else position={gx*CELL+.75,2.75,gz*CELL+1.0};
            const double range=std::hypot(position.x-listener.x,position.z-listener.z);
            if(range<4||range>18||!world.chunks().count(chunkAt(position.x,position.z)))continue;
            const float loss=transmission(world,listener,position);
            if(loss<.12f)continue;
            return ParkingAudioCue{sound,position,gain,loss,-1};
        }
        return {};
    }
};
}
