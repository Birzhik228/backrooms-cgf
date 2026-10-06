#include "../common/threat.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
constexpr float PI=3.14159265358979323846f;
void check(bool value,const std::string& message) { if (!value) throw std::runtime_error(message); }
void tick(br::Threat& threat,const br::World& world,const br::ThreatPlayer& player,float seconds) {
    for (int i=0;i<static_cast<int>(std::ceil(seconds*60));++i) {
        threat.update(world,player,1.f/60);
        check(threat.pathNodesVisited()<=br::Threat::MAX_PATH_NODES,"path search respects its fixed node budget");
        check(threat.pathPointCount()<=br::Threat::MAX_PATH_NODES+1,"retained route has a fixed bound");
        if (threat.active()) check(!world.blocked(threat.x(),threat.z(),br::Threat::RADIUS),"movement keeps the entire capsule clear");
    }
}

void perceptionAndCatch() {
    br::World world(42); world.update(3.75,3.75);
    br::Threat enemy(42);
    br::ThreatPlayer player; player.x=12; player.z=3.75;
    enemy.resetAt(3.75,3.75,PI/2);
    enemy.update(world,player,1.f/60);
    check(enemy.canSeePlayer() && enemy.state()==br::ThreatState::Chase,"front cone spots a still player across the entrance hall");
    check(enemy.speed()>0,"spotting starts a real chase");

    enemy.resetAt(3.75,3.75,-PI/2);
    enemy.update(world,player,1.f/60);
    check(!enemy.canSeePlayer() && enemy.state()==br::ThreatState::Wander,"a quiet player behind the entity is not seen");
    player.speed=4;
    enemy.resetAt(3.75,3.75,-PI/2);
    enemy.update(world,player,1.f/60);
    check(!enemy.canSeePlayer() && enemy.state()==br::ThreatState::Chase,"running behind the entity is heard and starts a chase");
    player.crouched=true; player.speed=1.1f;
    enemy.resetAt(3.75,3.75,-PI/2);
    enemy.update(world,player,1.f/60);
    check(enemy.state()==br::ThreatState::Wander,"crouched steps have a shorter hearing distance");
    player.speed=0; player.noise=1;
    enemy.resetAt(3.75,3.75,-PI/2);
    enemy.update(world,player,1.f/60);
    check(enemy.state()==br::ThreatState::Chase,"a nearby door event is heard independently of footstep speed");

    player={}; player.x=4.2; player.z=3.75;
    enemy.resetAt(3.75,3.75,PI/2);
    enemy.update(world,player,1.f/60);
    check(enemy.caught() && enemy.state()==br::ThreatState::Caught,"physical contact triggers caught state");
    const double x=enemy.x(),z=enemy.z();
    player.x=20; tick(enemy,world,player,.25f);
    check(enemy.x()==x && enemy.z()==z && enemy.caught(),"caught state remains frozen until explicit restart");
    enemy.reset(77);
    check(!enemy.caught() && !enemy.active() && !enemy.canSeePlayer() && enemy.pathPointCount()==0,"restart clears all threat state");
    std::cout<<"Perception, crouch, hearing, catch, reset: PASS\n";
}

void occlusionAndDoor() {
    br::World world(42); world.update(3.75,3.75);
    // A wall exists on the north edge of the entrance except at its central opening.
    check(!world.blocked(.9,-.6,br::Threat::RADIUS) && !world.blocked(.9,.6,br::Threat::RADIUS),"opposite wall test positions are walkable");
    check(!br::Threat::visibleSegment(world,.9,-.6,.9,.6),"solid wall blocks vision");
    br::Threat enemy; enemy.resetAt(.9,.6,0);
    br::ThreatPlayer player; player.x=.9; player.z=-.6;
    enemy.update(world,player,1.f/60);
    check(!enemy.canSeePlayer() && !enemy.caught(),"a player behind a close wall cannot be seen or caught through it");
    player.noise=1; enemy.update(world,player,1.f/60);
    check(enemy.state()==br::ThreatState::Chase,"nearby loud sounds cross walls at reduced range");

    br::DoorInfo door{};
    check(world.nearestDoor(3,3,14,door),"test optional door is present");
    const double nx=door.east?1:0,nz=door.east?0:1;
    const double ax=door.x-nx*1.2,az=door.z-nz*1.2;
    const double bx=door.x+nx*1.2,bz=door.z+nz*1.2;
    check(!world.blocked(ax,az,br::Threat::RADIUS) && !world.blocked(bx,bz,br::Threat::RADIUS),"door approach fixtures are free");
    check(!br::Threat::visibleSegment(world,ax,az,bx,bz),"closed door occludes sight");
    enemy.resetAt(ax,az,static_cast<float>(std::atan2(nx,-nz)));
    player={}; player.x=bx; player.z=bz; player.noise=1;
    tick(enemy,world,player,.6f);
    check(!enemy.caught(),"chase cannot phase through a closed door");
    check(world.toggleDoor(door.id,ax,az),"door opens for sight test");
    world.advanceDoors(.08f,ax,az);
    check(!br::Threat::visibleSegment(world,ax,az,bx,bz),"barely opened animated door still occludes its occupied plane");
    for(int i=0;i<120;++i) world.advanceDoors(1.f/60,ax,az);
    check(br::Threat::visibleSegment(world,ax,az,bx,bz),"fully opened door exposes the corridor");
    enemy.resetAt(ax,az,static_cast<float>(std::atan2(nx,-nz)));
    player.noise=0;
    tick(enemy,world,player,1.5f);
    check(enemy.caught(),"chase traverses an open door and catches on contact");
    std::cout<<"Walls, closed and animated doors, open-door pursuit: PASS\n";
}

void memoryPauseAndNavigation() {
    br::World world(42); world.update(3.75,3.75);
    br::Threat enemy; enemy.resetAt(3.75,3.75,PI/2);
    br::ThreatPlayer player; player.x=12; player.z=3.75;
    enemy.update(world,player,1.f/60);
    const double x=enemy.x(),z=enemy.z(); const float yaw=enemy.yaw(),memory=enemy.timeSinceSense();
    for(int i=0;i<400;++i) enemy.update(world,player,.1f,true);
    check(enemy.x()==x && enemy.z()==z && enemy.yaw()==yaw && enemy.timeSinceSense()==memory,"pause freezes animation state, navigation and memory");
    // Moving the player to a distant loaded room removes all sensory contact.
    player.x=60; player.z=60;
    tick(enemy,world,player,2);
    check(enemy.state()==br::ThreatState::Search && enemy.timeSinceSense()>1.9f,"lost contact searches the last known location");
    check(enemy.x()<13,"entity does not track an unseen player through the map");
    tick(enemy,world,player,3.3f);
    check(enemy.state()==br::ThreatState::Wander,"five seconds without senses returns to wandering");

    // This north wall forces a route through the reserved centre doorway.
    enemy.resetAt(.9,.7,0); player.x=.9; player.z=-.7; player.noise=1;
    bool usedPath=false;
    for(int i=0;i<360 && !enemy.caught();++i) {
        enemy.update(world,player,1.f/60);
        usedPath=usedPath || enemy.pathNodesVisited()>0;
        check(!world.blocked(enemy.x(),enemy.z(),br::Threat::RADIUS),"routing around walls never enters collision");
        check(enemy.pathNodesVisited()<=br::Threat::MAX_PATH_NODES,"path expansion stays bounded on detours");
    }
    check(usedPath && enemy.caught(),"enemy follows a nontrivial route around a wall to a heard player");
    std::cout<<"Pause, last-known search, five-second forgetting, bounded detour: PASS\n";
}

void safeSpawnAndStreaming() {
    br::World world(42); world.update(3.75,3.75);
    br::Threat first(42),second(42);
    br::ThreatPlayer player; player.x=3.75; player.z=3.75;
    tick(first,world,player,7);
    check(!first.active(),"new game provides an initial grace interval");
    for(int i=0;i<100;++i) first.update(world,player,.1f,true);
    check(!first.active(),"pause does not consume initial grace");
    for(int i=0;i<1200 && !first.active();++i) first.update(world,player,1.f/60);
    for(int i=0;i<1800 && !second.active();++i) second.update(world,player,1.f/60);
    check(first.active() && second.active(),"reachable safe spawn is found");
    check(std::hypot(first.x()-player.x,first.z()-player.z)>=br::Threat::SPAWN_MIN_DISTANCE,"closer entity still appears at least 16 m from player");
    check(std::hypot(first.x()-player.x,first.z()-player.z)<=br::Threat::SPAWN_MAX_DISTANCE+.01,"entity appears within 22 m for earlier encounters");
    check(first.x()==second.x() && first.z()==second.z(),"seed and initial player pose deterministically reproduce a spawn");
    check(first.pathPointCount()>0,"spawn proves a navigable route to player");
    const double dx=first.x()-player.x,dz=first.z()-player.z,dist=std::hypot(dx,dz);
    check((dx*player.forwardX+dz*player.forwardZ)/dist<.25 || !br::Threat::visibleSegment(world,player.x,player.z,first.x(),first.z()),"spawn is outside the generous view cone or behind geometry");

    br::Threat watched(42); watched.resetAt(85,3.75,PI/2);
    player.forwardX=1; player.forwardZ=0;
    check(br::Threat::visibleSegment(world,player.x,player.z,watched.x(),watched.z()),"reserved east corridor provides a long visible relocation fixture");
    watched.update(world,player,.1f);
    check(std::hypot(watched.x()-85,watched.z()-3.75)<=.111,"a distant but visible entity never teleports");
    player.forwardX=-1;
    for(int i=0;i<1200 && std::hypot(watched.x()-player.x,watched.z()-player.z)>75;++i)
        watched.update(world,player,1.f/60);
    check(std::hypot(watched.x()-player.x,watched.z()-player.z)<=br::Threat::SPAWN_MAX_DISTANCE+.01,"turning away permits safe recycling into the closer spawn band");
    player.forwardX=0; player.forwardZ=-1;

    // Change the streamed working set entirely. The same one actor must be
    // safely recycled into resident space instead of walking through void.
    player.x=903.75; player.z=3.75; world.update(player.x,player.z);
    for(int i=0;i<1200 && std::hypot(first.x()-player.x,first.z()-player.z)>75;++i)
        first.update(world,player,1.f/60);
    check(std::hypot(first.x()-player.x,first.z()-player.z)<=br::Threat::SPAWN_MAX_DISTANCE+.01,"one entity follows distant stream changes into the closer spawn band");
    check(!world.blocked(first.x(),first.z(),br::Threat::RADIUS),"relocated entity stays on loaded walkable floor");
    check(!br::Threat::visibleSegment(world,player.x,player.z,player.x+500,player.z),"nonresident geometry cannot be traversed or seen through by navigation");
    std::cout<<"Grace, deterministic reachable safe spawn, endless streaming: PASS\n";
}

void unreachableBudgetAndSeedSweep() {
    br::World world(42);
    const auto exit=world.exitLocation(); world.update(exit.x,exit.z);
    br::Threat enemy; enemy.resetAt(exit.x,exit.z,0);
    br::ThreatPlayer player; player.x=exit.x; player.z=exit.z-2.7; player.noise=1;
    check(!world.blocked(player.x,player.z,br::Threat::RADIUS),"exit chamber target is free but enclosed by a shut door");
    enemy.update(world,player,1.f/60);
    check(enemy.state()==br::ThreatState::Chase && enemy.pathPointCount()==0,"unreachable sound causes pursuit intent without inventing a path");
    check(enemy.pathNodesVisited()==br::Threat::MAX_PATH_NODES,"unreachable route stops at the exact expansion budget");
    check(!enemy.caught() && !world.blocked(enemy.x(),enemy.z(),br::Threat::RADIUS),"budget exhaustion leaves entity safely at its current position");

    for (const std::uint64_t seed:{0ULL,1ULL,13ULL,91ULL,777ULL}) {
        br::World varied(seed); varied.update(3.75,3.75);
        br::Threat actor(seed); br::ThreatPlayer observer; observer.x=observer.z=3.75;
        for(int i=0;i<1800 && !actor.active();++i) actor.update(varied,observer,1.f/60);
        check(actor.active() && !varied.blocked(actor.x(),actor.z(),br::Threat::RADIUS),"safe reachable spawn succeeds across varied seeds");
        const double distance=std::hypot(actor.x()-observer.x,actor.z()-observer.z);
        check(distance>=br::Threat::SPAWN_MIN_DISTANCE && distance<=br::Threat::SPAWN_MAX_DISTANCE,
              "all seeds use the closer 16 to 22 m spawn band");
        const double forwardDot=((actor.x()-observer.x)*observer.forwardX+(actor.z()-observer.z)*observer.forwardZ)/distance;
        check(forwardDot<.25 || !br::Threat::visibleSegment(varied,observer.x,observer.z,actor.x(),actor.z()),
              "closer spawn does not pop into the player's view across varied seeds");
        check(actor.pathPointCount()>0 && actor.pathNodesVisited()<=br::Threat::MAX_PATH_NODES,"each seeded spawn has a bounded route");
    }
    std::cout<<"Unreachable path budget and varied-seed reachable spawning: PASS\n";
}
}

int main() {
    try {
        perceptionAndCatch(); occlusionAndDoor(); memoryPauseAndNavigation(); safeSpawnAndStreaming(); unreachableBudgetAndSeedSweep();
        std::cout<<"Threat tests: PASS\n"; return 0;
    } catch (const std::exception& error) {
        std::cerr<<"Threat tests: FAIL: "<<error.what()<<'\n'; return 1;
    }
}
