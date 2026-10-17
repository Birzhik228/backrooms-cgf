#include "world.h"
#include "common/threat.h"
#include "common/mouse.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
void check(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
int main() { try {
    for(unsigned seed:{0u,1u,42u,991u,12071998u}) {
        br::World w(seed);auto exit=w.exitLocation();w.update(exit.x,exit.z);
        bool darkExit=false;
        for(const auto& entry:w.chunks())for(const auto& v:entry.second.vertices)darkExit|=v.material==57;
        check(darkExit,"exit has a dark interior lining");
        br::DoorInfo door{};check(w.nearestDoor(exit.x,exit.z,2,door),"exit exists");
        check(!w.toggleDoor(door.id,exit.x,exit.z),"exit starts locked");
        auto key=w.keyLocation();check(std::hypot(key.x-exit.x,key.z-exit.z)>12 && std::hypot(key.x-exit.x,key.z-exit.z)<42,"key is nearby on a connected route");
        check(!w.collectKey(exit.x,exit.z),"remote pickup rejected");
        w.update(key.x,key.z);check(!w.blocked(key.x,key.z,.25),"key reachable on route");
        check(!w.keyVertices(0,0).empty(),"uncollected key renders");
        for(const auto& vertex:w.keyVertices(0,0)) check(vertex.y>=0 && vertex.y<.04f && vertex.material==24,"key rests on floor without a plinth");
        check(w.collectKey(key.x,key.z) && !w.collectKey(key.x,key.z),"pickup fires once");
        check(w.keyVertices(0,0).empty(),"collected key disappears");
        w.update(exit.x,exit.z);check(w.nearestDoor(exit.x,exit.z,2,door) && !door.open && door.openness==0,"key leaves distant exit closed");
        check(!w.exitCrossed(door.x,door.z-.8),"unlocked closed exit cannot be crossed");
        w.update(6000,6000);w.update(exit.x,exit.z);
        check(w.nearestDoor(exit.x,exit.z,2,door) && !door.open && door.openness==0,"unlocked closed pose survives eviction");
        check(w.toggleDoor(door.id,exit.x,exit.z),"E opens unlocked exit");
        for(int i=0;i<60;++i)w.advanceDoors(.016f,exit.x,exit.z);
        check(w.exitCrossed(door.x,door.z-.8),"unlocked exit is usable");
        w.update(6000,6000);w.update(exit.x,exit.z);
        check(w.keyCollected() && w.exitCrossed(door.x,door.z-.8),"progress survives streaming");
        w=br::World(seed);check(!w.keyCollected(),"new run resets key");
        w.update(3.75,-20);bool recess=false;
        for(auto& [coord,chunk]:w.chunks()) for(auto& spot:chunk.hidingSpots) {
            (void)coord;recess=true;
            check(!w.blocked(spot.inside.x,spot.inside.z,.23),"hide interior is clear");
            check(!w.blocked(spot.approach.x,spot.approach.z,.23),"hide approach is clear");
            check(w.headClearance(spot.inside.x,spot.inside.z,.23)>=1.3,"crouched camera fits");
            check(w.headClearance(spot.inside.x,spot.inside.z,.23)<1.5,"standing player cannot enter low pocket");
            const double dx=(spot.inside.x-spot.approach.x)/1.38,dz=(spot.inside.z-spot.approach.z)/1.38;
            check(w.blocked(spot.inside.x+dx*.6,spot.inside.z+dz*.6,.23),"pocket retains a solid backing wall");
            check(w.blocked(spot.inside.x-dx*.49,spot.inside.z-dz*.49,.10),"closed vent cover blocks walking; E enters behind it");
        }
        check(recess,"seeded wall recesses are present");
        check(w.soundTransmission(3.75,-10,3.75,-8)>.99,"open route transmits sound");
        bool muffled=false;
        for(auto& [coord,chunk]:w.chunks()) for(auto& spot:chunk.hidingSpots)
            muffled|=w.soundTransmission(spot.inside.x,spot.inside.z,spot.inside.x+(spot.inside.x-spot.approach.x),spot.inside.z+(spot.inside.z-spot.approach.z))<.5;
        check(muffled,"walls attenuate sound");
    }
    br::World w(42);w.update(3.75,3.75);
    bool checkedDoor=false;
    for(const auto& [coord,chunk]:w.chunks()) {
        (void)coord;
        for(const auto door:chunk.doors) {
            if(door.isExit || checkedDoor) continue;
            const double dx=door.east?1:0,dz=door.east?0:1;
            if(w.blocked(door.x-dx,door.z-dz,.25) || w.blocked(door.x+dx,door.z+dz,.25)) continue;
            const float closed=w.soundTransmission(door.x-dx,door.z-dz,door.x+dx,door.z+dz);
            check(w.toggleDoor(door.id,door.x-dx,door.z-dz),"acoustic fixture opens");
            for(int i=0;i<60;++i) w.advanceDoors(.016f,10000,10000);
            check(w.soundTransmission(door.x-dx,door.z-dz,door.x+dx,door.z+dz)>closed*2,"open doors restore transmission");
            checkedDoor=true;
        }
    }
    check(checkedDoor,"closed-door acoustic fixture tested");
    br::Threat normal(42),fast(42);
    normal.resetAt(3.75,-5,3.14159265f);fast.resetAt(3.75,-5,3.14159265f);fast.setEnraged(true);
    br::ThreatPlayer p{3.75,3.75,0,-1,0,false,0};
    for(int i=0;i<60;++i) {normal.update(w,p,1.f/60);fast.update(w,p,1.f/60);}
    check(fast.z()>normal.z()+.5,"optional rage modifier increases pursuit speed");
    fast.resetAt(3.75,3.3,3.14159265f);p.hidden=true;
    for(int i=0;i<360;++i) fast.update(w,p,1.f/60);
    check(!fast.caught() && !fast.canSeePlayer(),"hidden player is neither sensed nor caught");
    fast.reset(42);check(!fast.enraged(),"reset clears rage");
    br::Mouse mouse;mouse.reset(42);
    check(!mouse.update(w,3.75,3.75,.016f,true,false) && !mouse.active,"pause freezes mouse initialization");
    mouse.update(w,3.75,3.75,.016f,false,false);
    check(mouse.active && !w.blocked(mouse.x,mouse.z,.12),"mouse spawns on walkable floor");
    const double mx=mouse.x,mz=mouse.z;
    check(!mouse.update(w,mx,mz,.016f,false,true),"hidden player does not trigger mouse");
    check(mouse.update(w,mouse.x,mouse.z,.016f,false,false),"contact squeaks");
    for(int i=0;i<240;++i) check(!mouse.update(w,mouse.x,mouse.z,1.f/60,false,false),"sustained contact cannot spam squeaks");
    mouse.update(w,mouse.x+2,mouse.z,.016f,false,false);
    check(!mouse.update(w,mouse.x,mouse.z,.016f,false,false),"one alarm per mouse lifetime, even after separate contact");
    br::Mouse seeker;seeker.reset(42);seeker.active=true;seeker.x=3.75;seeker.z=.75;
    for(int i=0;i<60;++i)seeker.update(w,3.75,3.75,1.f/60,false,false);
    check(seeker.z>1.8 && !seeker.alerted,"mouse actively approaches player");
    seeker.x=3.75;seeker.z=3.4;
    check(seeker.update(w,3.75,3.75,1.f/60,false,false) && seeker.alerted,"first contact permanently changes to fleeing");
    for(int i=0;i<60;++i)check(!seeker.update(w,3.75,3.75,1.f/60,false,false),"fleeing mouse cannot summon twice");
    check(std::hypot(seeker.x-3.75,seeker.z-3.75)>1.5,"mouse escapes after alarm");
    const auto mesh=mouse.vertices(0,0);check(!mesh.empty(),"mouse has a visible model");
    for(const auto& v:mesh) check(std::isfinite(v.x) && std::isfinite(v.nx) && v.y>=-.001f,"mouse mesh stays grounded and finite");
    mouse.reset(42);check(!mouse.active,"restart resets mouse");
    int quietCues=0;
    for(int i=0;i<600;++i) {
        check(!mouse.update(w,3.75,3.75,1.f/60,false,true),"ordinary mouse sounds never become contact alarms");
        if(mouse.ambientCue) ++quietCues;
    }
    check(quietCues>=2 && quietCues<=3,"quiet discovery sounds recur at a restrained cadence");
    const auto frozen=mouse.vertices(0,0);
    for(int i=0;i<120;++i) {
        mouse.update(w,3.75,3.75,1.f/60,true,false);
        check(!mouse.ambientCue,"pause does not replay ambient cues");
    }
    const auto pausedMesh=mouse.vertices(0,0);
    for(std::size_t i=0;i<frozen.size();++i)
        check(frozen[i].x==pausedMesh[i].x && frozen[i].y==pausedMesh[i].y && frozen[i].z==pausedMesh[i].z,"pause freezes mouse pose");
    bool bodyMoved=false;
    for(int i=0;i<30;++i) mouse.update(w,3.75,3.75,1.f/60,false,true);
    const auto animatedMesh=mouse.vertices(0,0);
    for(std::size_t i=0;i<frozen.size();++i) {
        bodyMoved|=std::abs(frozen[i].y-animatedMesh[i].y)>.001f;
        check(animatedMesh[i].y>=-.001f,"animated paws and body stay above floor");
    }
    check(bodyMoved,"mouse pose animates beyond horizontal translation");
    br::Threat alerted(42);alerted.resetAt(3.75,-5,0);
    p.hidden=true; // Isolate alarm response from ordinary sight and footsteps.
    alerted.hearAlarm(w,p,3.75,0);
    const double before=alerted.z();
    for(int i=0;i<30;++i) alerted.update(w,p,1.f/60);
    check(alerted.state()==br::ThreatState::Search && alerted.z()>before,"squeak attracts creature to sound location");
    alerted.resetAt(3.75,-5,0);alerted.setEnraged(true);
    alerted.hearAlarm(w,p,3.75,0,true);
    for(int i=0;i<30;++i)alerted.update(w,p,1.f/60);
    check(alerted.state()==br::ThreatState::Search && alerted.z()>-5 && alerted.enraged(),"key call starts investigation with persistent speed boost");
    p.x=200;p.z=200; // Moving the hidden player must not refresh the call.
    for(int i=0;i<360;++i)alerted.update(w,p,1.f/60);
    check(alerted.state()==br::ThreatState::Wander && alerted.enraged(),"key call expires without tracking while speed boost remains");
    p.x=3.75;p.z=3.75;
    alerted.reset(42);alerted.hearAlarm(w,p,3.75,3.75);alerted.update(w,p,.016f);
    check(alerted.active(),"alarm can summon dormant creature without waiting for grace period");
    std::cout<<"Progression tests passed: key gate, streaming/reset, hiding geometry, wall attenuation, rage and hidden perception.\n";
} catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;} }
