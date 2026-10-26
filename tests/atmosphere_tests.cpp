#include "../common/atmosphere.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr float DT=.05f;
constexpr double PI=3.14159265358979323846;

void check(bool value,const std::string& message) {
    if(!value)throw std::runtime_error(message);
}

struct TimedCue { double time; br::AtmosphereCue cue; };

std::vector<TimedCue> simulate(br::Level0Atmosphere& director,const br::World& world,
                               const br::AtmosphereContext& context,double seconds) {
    std::vector<TimedCue> result;
    for(int frame=0;frame<int(std::lround(seconds/DT));++frame) {
        auto cue=director.update(world,context,DT);
        if(cue)result.push_back({(frame+1)*double(DT),*cue});
    }
    return result;
}

bool same(const std::optional<br::AtmosphereCue>& left,const std::optional<br::AtmosphereCue>& right) {
    if(bool(left)!=bool(right))return false;
    return !left || (left->kind==right->kind && left->position.x==right->position.x &&
        left->position.y==right->position.y && left->position.z==right->position.z &&
        left->gain==right->gain && left->transmission==right->transmission);
}

bool ambient(br::SoundKind kind) {
    return kind==br::SoundKind::VentRattle || kind==br::SoundKind::AmbientCreak ||
           kind==br::SoundKind::WaterDrip;
}

// Find physical fixtures from generated collision geometry, without assuming a
// particular room layout or replacing the real World with a mock.
br::AudioPosition findSource(const br::World& world,br::AudioPosition listener,
                            double minimum,double maximum,int wantedWalls) {
    for(double distance=minimum;distance<=maximum;distance+=.5) {
        for(int degrees=0;degrees<360;degrees+=5) {
            const double angle=degrees*PI/180;
            br::AudioPosition source{listener.x+std::cos(angle)*distance,.2,
                                     listener.z+std::sin(angle)*distance};
            if(world.blocked(source.x,source.z,.25))continue;
            const float transmission=world.soundTransmission(listener.x,listener.z,source.x,source.z);
            if(std::abs(transmission-std::pow(.38f,float(wantedWalls)))<.00001)return source;
        }
    }
    throw std::runtime_error("bounded geometry search could not find an acoustic fixture");
}

int wallSpans(const br::World& world,br::AudioPosition listener,br::AudioPosition source) {
    const double distance=std::hypot(source.x-listener.x,source.z-listener.z);
    const int samples=int(std::ceil(distance/.1));
    int spans=0;bool previous=false;
    for(int i=1;i<samples;++i) {
        const bool solid=world.blocked(listener.x+(source.x-listener.x)*i/samples,
                                       listener.z+(source.z-listener.z)*i/samples,.06);
        if(solid && !previous)++spans;
        previous=solid;
    }
    return spans;
}

void validatePlacement(const br::World& world,const br::AtmosphereContext& context,
                       const std::vector<TimedCue>& cues) {
    double previous=-100;
    for(const auto& item:cues) {
        const auto& cue=item.cue;
        check(item.time-previous>=7-.001,"independent atmosphere events are at least seven seconds apart");
        previous=item.time;
        check(std::isfinite(cue.position.x) && std::isfinite(cue.position.y) && std::isfinite(cue.position.z),
              "all source coordinates are finite");
        check(!world.blocked(cue.position.x,cue.position.z,.18),"source occupies resident walkable geometry");
        check(cue.gain>0 && cue.gain<=1,"cue has bounded audible gain");
        const double distance=std::hypot(cue.position.x-context.listener.x,cue.position.z-context.listener.z);
        const float actual=world.soundTransmission(context.listener.x,context.listener.z,cue.position.x,cue.position.z);
        check(std::abs(cue.transmission-actual)<.00001,"cue carries actual wall attenuation");
        if(cue.kind==br::SoundKind::DistantFootsteps) {
            check(distance>=7-.0001 && distance<=16+.0001,"distant footsteps remain seven to sixteen metres away");
            check(cue.position.y>=0 && cue.position.y<.5,"footsteps originate at floor height");
            const int walls=wallSpans(world,context.listener,cue.position);
            check(walls>=1 && walls<=2,"every distant footstep has one or two physical barriers");
            check(cue.transmission>=.12f && cue.transmission<=.5f,"distant footsteps remain muffled but audible");
        } else if(cue.kind==br::SoundKind::ThreatRustle) {
            check(context.threatActive,"a dormant creature cannot create a presence cue");
            check(cue.position.x==context.threat.x && cue.position.z==context.threat.z,
                  "presence cue originates at the actual creature, not an invented position");
            check(distance>=10-.0001 && distance<=22+.0001,"presence hint stays within its distant hearing band");
            check(wallSpans(world,context.listener,cue.position)==1,"presence hint has exactly one physical barrier");
        } else {
            check(ambient(cue.kind),"director only emits approved environmental sound kinds");
            check(distance>=4-.0001 && distance<=10+.0001,"room foley stays within the nearby environment");
        }
    }
}

void sparseEnvironment() {
    for(const auto seed:{42ULL,12071998ULL,2026ULL}) {
        br::World world(seed);world.update(3.75,3.75);
        br::AtmosphereContext state;state.listener={3.75,1.65,3.75};
        br::Level0Atmosphere director(seed);
        const auto cues=simulate(director,world,state,240);
        validatePlacement(world,state,cues);
        std::set<br::SoundKind> environmental;
        int steps=0;double lastSteps=-100;
        for(const auto& item:cues) {
            if(ambient(item.cue.kind))environmental.insert(item.cue.kind);
            if(item.cue.kind==br::SoundKind::DistantFootsteps) {
                check(item.time-lastSteps>=40,"wall footsteps remain rare during a long quiet session");
                lastSteps=item.time;++steps;
            }
            check(item.cue.kind!=br::SoundKind::ThreatRustle,"dormant Aranea does not produce a ghost presence hint");
        }
        check(cues.size()>=7 && cues.size()<=24,"quiet four-minute session is neither silent nor saturated");
        check(environmental.size()==3,"quiet session uses ventilation, creaks and water rather than one repeated sound");
        check(steps>=2 && steps<=5,"quiet session includes a small number of wall footstep episodes");
    }
    std::cout<<"Sparse environment: three seeded worlds, real barriers and varied room foley PASS\n";
}

void groundedPresence() {
    br::World world(12071998);world.update(3.75,3.75);
    br::AtmosphereContext state;state.listener={3.75,1.65,3.75};state.threatActive=true;
    state.threat=findSource(world,state.listener,12,20,1);
    br::Level0Atmosphere director(17);
    auto cues=simulate(director,world,state,180);
    validatePlacement(world,state,cues);
    int presence=0;
    for(const auto& item:cues)presence+=item.cue.kind==br::SoundKind::ThreatRustle;
    check(presence>=2 && presence<=6,"real occluded Aranea produces occasional presence sounds");

    const auto occluded=state.threat;
    for(int fixture=0;fixture<4;++fixture) {
        state.threatActive=fixture!=0;
        state.threat=fixture==1?findSource(world,state.listener,10,22,0):
                     fixture==2?findSource(world,state.listener,23,28,1):
                     fixture==3?findSource(world,state.listener,10,22,2):occluded;
        director.reset(17);
        cues=simulate(director,world,state,120);
        for(const auto& item:cues)
            check(item.cue.kind!=br::SoundKind::ThreatRustle,
                  "inactive, unobstructed, too distant or heavily occluded Aranea has no presence hint");
    }
    std::cout<<"Presence: actual occluded creature only, no dormant or ungrounded hints PASS\n";
}

void clockAndSafety() {
    br::World world(42);world.update(3.75,3.75);
    br::AtmosphereContext safe;safe.listener={3.75,1.65,3.75};
    br::Level0Atmosphere control(99),interrupted(99);
    for(int frame=0;frame<4800;++frame) {
        if(frame==100 || frame==2200) {
            auto paused=safe;paused.paused=true;
            check(simulate(interrupted,world,paused,90).empty(),"paused time emits no atmosphere");
        }
        check(same(control.update(world,safe,DT),interrupted.update(world,safe,DT)),
              "pause freezes schedule and random sequence exactly");
    }
    for(const auto level:{br::Level::Fun,br::Level::Parking}) {
        br::World other(42,level);other.update(3.75,3.75,0);
        br::Level0Atmosphere director(42);
        check(simulate(director,other,safe,240).empty(),"Level Fun and Parking receive no Level 0 atmosphere");
    }
    for(int reason=0;reason<5;++reason) {
        br::Level0Atmosphere director(123);
        simulate(director,world,safe,25);
        auto unsafe=safe;
        if(reason==0)unsafe.hidden=true;
        if(reason==1)unsafe.threatVisible=true;
        if(reason==2)unsafe.threatDanger=true;
        if(reason==3){unsafe.threatActive=true;unsafe.threat={4.75,1.2,3.75};}
        if(reason==4)unsafe.alive=false;
        check(simulate(director,world,unsafe,90).empty(),"hiding, sighting, danger, close creature and death suppress cues");
        if(reason!=4) {
            check(simulate(director,world,safe,8).empty(),"danger recovery provides at least eight quiet seconds");
            const auto recovered=simulate(director,world,safe,120);
            check(!recovered.empty(),"atmosphere resumes after recovery");
            validatePlacement(world,safe,recovered);
        }
    }
    std::cout<<"Safety: pause invariance, level isolation, danger suppression and quiet recovery PASS\n";
}

void determinismAndInvalidInputs() {
    br::World world(42);world.update(3.75,3.75);
    br::AtmosphereContext state;state.listener={3.75,1.65,3.75};
    br::Level0Atmosphere reset(1),fresh(456);
    simulate(reset,world,state,170);reset.reset(456);
    for(int frame=0;frame<4800;++frame)
        check(same(reset.update(world,state,DT),fresh.update(world,state,DT)),"reset restores seeded sequence after prior activity");

    br::Level0Atmosphere normal(71),invalid(71);
    for(const float dt:{0.f,-1.f,std::numeric_limits<float>::infinity(),
                       -std::numeric_limits<float>::infinity(),std::numeric_limits<float>::quiet_NaN()})
        for(int i=0;i<100;++i)check(!invalid.update(world,state,dt),"invalid timestep cannot emit an event");
    auto broken=state;
    for(int coordinate=0;coordinate<3;++coordinate) {
        broken=state;
        if(coordinate==0)broken.listener.x=std::numeric_limits<double>::infinity();
        if(coordinate==1)broken.listener.y=std::numeric_limits<double>::quiet_NaN();
        if(coordinate==2)broken.listener.z=-std::numeric_limits<double>::infinity();
        check(!invalid.update(world,broken,DT),"invalid listener cannot emit an event");
    }
    for(int frame=0;frame<2400;++frame)
        check(same(normal.update(world,state,DT),invalid.update(world,state,DT)),"rejected inputs leave timing and randomness untouched");

    br::Level0Atmosphere stalled(71);
    check(!stalled.update(world,state,1000000.f),"a large stall cannot immediately release scheduled sounds");
    std::cout<<"Robustness: seed reset, rejected inputs and stall protection PASS\n";
}

void distantWorldCoordinates() {
    constexpr double offset=1e9;
    br::World world(42);world.update(offset,offset);
    br::AtmosphereContext state;
    bool found=false;
    for(int x=-8;x<=8 && !found;++x)for(int z=-8;z<=8 && !found;++z) {
        const double wx=offset+x,wz=offset+z;
        if(!world.blocked(wx,wz,.25)){state.listener={wx,1.65,wz};found=true;}
    }
    check(found,"large-coordinate fixture has a clear listener position");
    br::Level0Atmosphere director(402);
    const auto cues=simulate(director,world,state,180);
    check(!cues.empty(),"atmosphere remains active far from the rendering origin");
    validatePlacement(world,state,cues);
    for(const auto& item:cues)
        check(std::abs(item.cue.position.x-offset)<30 && std::abs(item.cue.position.z-offset)<30,
              "sources retain global coordinates rather than snapping to the origin");
    std::cout<<"Precision: bounded sound placement at billion-metre world coordinates PASS\n";
}
}

int main() {
    try {
        sparseEnvironment();groundedPresence();clockAndSafety();determinismAndInvalidInputs();distantWorldCoordinates();
        std::cout<<"All Level 0 atmosphere tests passed.\n";
    } catch(const std::exception& error) {
        std::cerr<<"Atmosphere test failed: "<<error.what()<<'\n';return 1;
    }
}
