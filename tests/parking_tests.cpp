#include "world.h"
#include "common/partygoer.h"
#include "common/darkness.h"
#include <cmath>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>

void check(bool ok,const char* message) { if(!ok)throw std::runtime_error(message); }
int main() {try {
    using namespace br;
    for(auto seed:{0ULL,3ULL,42ULL,12071998ULL}) {
        World w(seed,Level::Parking);w.update(3.75,3.75,1);
        check(w.isParking() && !w.isFun() && w.chunks().size()==9,"bounded independent parking world");
        for(int i=0;i<3;++i)for(int j=i+1;j<3;++j) {
            const auto a=w.breakerLocation(i),b=w.breakerLocation(j);
            check(std::hypot(a.x-b.x,a.z-b.z)>75,"cabinets occupy widely separated sectors");
        }
        check(!w.blocked(3.75,3.75,.25) && w.headClearance(3.75,3.75,.25)>2.5f,"safe standing spawn");
        check(!w.collectKey(w.keyLocation().x,w.keyLocation().z) && w.keyVertices(0,0).empty(),"no Level 0 key in parking");
        check(w.presentsOpened()==0 && !w.infiniteStamina(),"no party rewards leak into parking");
        Partygoers party;party.reset();party.update(w,3.75,3.75,1.65f,{0,0,-1},75,1.6f,1.f/60.f,false);
        for(const auto& actor:party.actors)check(!actor.active,"party actors disabled outside Fun");
        std::set<int> materials;
        for(const auto& entry:w.chunks()) {
            const auto& c=entry.second;
            check(c.presents.empty() && c.hidingSpots.empty(),"no obsolete interactions");
            check(!c.distantVertices.empty() && c.vertices.size()<180000,"bounded full and distant meshes");
            for(const auto& v:c.vertices) {
                check(std::isfinite(v.x)&&std::isfinite(v.nx)&&std::isfinite(v.ny)&&std::isfinite(v.nz),"finite parking geometry");
                check(v.y>=0 && v.y<=3.201 && v.x>=-.12 && v.x<=CHUNK_SIZE+.12 && v.z>=-.12 && v.z<=CHUNK_SIZE+.12,"props contained in chunk bounds");
                materials.insert(int(v.material));
            }
            for(std::size_t i=0;i<c.vertices.size();i+=3) {
                const auto &a=c.vertices[i],&b=c.vertices[i+1],&d=c.vertices[i+2];
                const double ux=b.x-a.x,uy=b.y-a.y,uz=b.z-a.z,vx=d.x-a.x,vy=d.y-a.y,vz=d.z-a.z;
                check((uy*vz-uz*vy)*a.nx+(uz*vx-ux*vz)*a.ny+(ux*vy-uy*vx)*a.nz>=-1e-6,"outward triangle winding");
            }
        }
        for(int mat:{70,71,72,73,74,75,76,80,89,90})check(materials.count(mat)>0,"garage architecture, cars, markings and fixtures present");
        // A north/south drive aisle and east/west cross aisle form an unbroken
        // network across positive and negative chunk seams, including landmarks.
        for(int z=-5;z<11;++z)for(int x=-5;x<11;++x) {
            for(int n=0;n<=30;++n) {
                check(!w.blocked((x+.5)*CELL,z*CELL+n*.25,.23),"continuous drive aisle");
                check(!w.blocked(x*CELL+n*.25,z*CELL+5.75,.23),"continuous cross aisle");
            }
        }
        auto exit=w.exitLocation();w.update(exit.x,exit.z,1);DoorInfo door{};
        check(w.nearestDoor(exit.x,exit.z,2,door)&&door.isExit,"service exit generated at its marker");
        check(!w.toggleDoor(door.id,exit.x,exit.z) && !w.exitCrossed(door.x,door.z-.8),"unpowered exit locked");
        for(int i:{2,0,1}) {
            auto p=w.breakerLocation(i);w.update(p.x,p.z,1);
            const auto cell=parkingBreakerCell(seed,i);
            check(parkingSector(cell.x,cell.z)==i,"cabinet matches its sector");
            for(int n=0;n<=30;++n)
                check(!w.blocked((cell.x+.5)*CELL+(5.825-CELL*.5)*n/30,p.z+1,.23),"reachable panel approach from drive aisle");
            check(!w.activateBreaker(i,p.x+20,p.z) && !w.activateBreaker(i,p.x,p.z-1),"remote and rear interactions rejected");
            check(!w.activateBreaker(-1,p.x,p.z+1) && !w.activateBreaker(3,p.x,p.z+1),"invalid circuit rejected");
            check(!w.activateBreaker(i,std::numeric_limits<double>::quiet_NaN(),p.z),"invalid coordinates rejected");
            w.prepareAhead(p.x,p.z);
            check(w.activateBreaker(i,p.x,p.z+1),"front interaction restores circuit");
            check(w.breakerPowered(i) && !w.activateBreaker(i,p.x,p.z+1),"activation is idempotent");
            for(const auto* chunks:{&w.chunks(),&w.preparedChunks()})for(const auto& entry:*chunks)
                for(const auto& lamp:entry.second.lamps) {
                    const auto gx=int64_t(std::floor((entry.first.x*CHUNK_SIZE+lamp.x)/CELL));
                    const auto gz=int64_t(std::floor((entry.first.z*CHUNK_SIZE+lamp.z)/CELL));
                    if(parkingSector(gx,gz)==i)check(lamp.state==LampState::Working,"resident and prefetched circuit lights restored");
                }
            if(w.breakersPowered()<3)check(!w.exitUnlocked(),"partial power cannot unlock exit");
        }
        check(w.exitUnlocked() && !w.keyCollected(),"three circuits unlock exit without a key");
        w.update(6000,-6000,0);w.update(exit.x,exit.z,1);
        check(w.breakersPowered()==3,"power survives chunk eviction");
        check(w.nearestDoor(exit.x,exit.z,2,door) && w.toggleDoor(door.id,exit.x,exit.z),"powered service exit opens");
        for(int i=0;i<70;++i)w.advanceDoors(1.f/60.f,exit.x,exit.z);
        check(!w.exitCrossed(exit.x,exit.z) && w.exitCrossed(door.x,door.z-.8),"must cross an open doorway to finish");
        check(!w.shiftBehind(exit.x,exit.z,0,-1),"parking landmarks never shuffle");
        w=World(seed,Level::Parking);check(w.breakersPowered()==0 && !w.exitUnlocked(),"new building resets circuit state");
    }
    World zero(42),fun(42,Level::Fun);
    check(!zero.activateBreaker(0,0,0) && !fun.activateBreaker(0,0,0),"circuits belong only to parking");
    World garage(42,Level::Parking);garage.update(3.75,-11.25,1);
    Darkness dark;
    auto& eyes=dark.eyes[0];eyes.active=true;eyes.x=3.75;eyes.z=-17.25;
    const double before=eyes.z;
    check(!dark.update(garage,3.75,-11.25,1.7,{0,0,-1},false,.016f,true) && eyes.z==before,"pause freezes eyes and contact");
    check(!dark.update(garage,3.75,-11.25,1.7,{0,0,-1},false,.016f,false) && eyes.active && eyes.z>before,"looking with flashlight off does not repel darkness");
    dark.update(garage,3.75,-11.25,1.7,{0,0,1},true,.016f,false);
    check(eyes.active,"flashlight aimed away cannot repel eyes");
    dark.update(garage,3.75,-11.25,1.7,{0,0,-1},true,.016f,false);
    check(!eyes.active && dark.dispelled==1 && eyes.cooldown>=4,"direct flashlight beam disperses eyes with respawn delay");
    eyes.active=true;eyes.x=8.8;eyes.z=6.8;eyes.y=1.7;
    check(!Darkness::inBeam(garage,eyes,4.8,1.7,6.8,{1,0,0},true),"concrete column occludes flashlight repulsion");
    eyes.x=3.75;eyes.z=-11.65;
    check(dark.update(garage,3.75,-11.25,1.7,{0,0,1},true,.016f,false),"unguarded contact in darkness is lethal");
    dark.reset();dark.eyes[0].active=true;dark.eyes[0].x=3.75;dark.eyes[0].z=3.8;
    check(!dark.update(garage,3.75,3.75,1.7,{0,0,-1},false,.016f,false) && !dark.eyes[0].active,"entry light disperses intruding eyes");
    for(int i=0;i<900;++i)check(!dark.update(garage,3.75,3.75,1.7,{0,0,-1},false,1.f/60.f,false),"entrance refuge protects without a flashlight");
    check(dark.spawned>0 && dark.activeCount()<=3,"bounded seeded eyes spawn after grace period");
    Darkness pursuit;pursuit.eyes[0].active=true;pursuit.eyes[0].x=3.75;pursuit.eyes[0].z=-19;
    pursuit.eyes[1].cooldown=pursuit.eyes[2].cooldown=100;
    double runnerZ=-23;bool caught=false;
    for(int i=0;i<420 && !caught;++i) {
        runnerZ-=4.0/60;garage.update(3.75,runnerZ,1);
        caught=pursuit.update(garage,3.75,runnerZ,1.7,{0,0,-1},false,1.f/60.f,false);
    }
    check(caught,"darkness overtakes a player sprinting at 4 m/s along a clear aisle");
    const auto face=Darkness::scareVertices({0,1.7,0},{0,0,-1},1);
    check(face.size()==6 && face[0].material==92,"capture has a dedicated close-up mesh");
    for(int i=0;i<3;++i) {const auto p=garage.breakerLocation(i);garage.update(p.x,p.z,1);check(garage.activateBreaker(i,p.x,p.z+1),"restore safe sectors for darkness test");}
    dark.update(garage,3.75,3.75,1.7,{0,0,-1},false,.016f,false);
    check(dark.activeCount()==0 && Darkness::safe(garage,3.75,-11.25),"restored circuits clear darkness");
    dark.eyes[0].active=true;dark.update(fun,0,0,1.7,{0,0,-1},false,.016f,false);
    check(dark.activeCount()==0,"darkness cannot leak into other levels");
    std::cout<<"Parking tests passed: four seeds, mesh bounds/winding, connected aisles, three circuit interactions, lighting, streaming and exit gating.\n";
    std::cout<<"Darkness tests passed: beam cone, flashlight off, occlusion, pause, approach, contact, refuges, powered sectors and level isolation.\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
