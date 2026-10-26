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

void checkForwardStep(const br::Threat& threat,double previousX,double previousZ,float previousYaw,float dt) {
    const double dx=threat.x()-previousX,dz=threat.z()-previousZ,distance=std::hypot(dx,dz);
    check(std::abs(std::remainder(threat.yaw()-previousYaw,2*PI))<=9*dt+1e-5,
          "all movement substeps share one bounded angular budget");
    if (distance>1e-7) {
        const double forwardDot=(dx*std::sin(threat.yaw())-dz*std::cos(threat.yaw()))/distance;
        check(forwardDot>=std::cos(2*PI/180)-1e-6,
              "each translated frame faces within two degrees of its actual travel direction");
    } else check(threat.speed()==0,"turning in place has zero gait speed");
}

void forwardLocomotion() {
    br::World world(42); world.update(3.75,3.75);
    // Four world directions catch model/AI conventions that only work north.
    for (const float heading:{0.f,PI/2,PI,-PI/2}) {
        br::ThreatPlayer player; player.x=3.75+2.85*std::sin(heading);
        player.z=3.75-2.85*std::cos(heading); player.noise=1;
        check(!world.blocked(player.x,player.z,br::Threat::RADIUS),"cardinal pursuit target is clear");
        br::Threat actor; actor.resetAt(3.75,3.75,heading);
        for (int i=0;i<3;++i) {
            const double x=actor.x(),z=actor.z(); const float yaw=actor.yaw();
            actor.update(world,player,.1f);
            checkForwardStep(actor,x,z,yaw,.1f);
            check(actor.speed()>3.2f,"aligned pursuit runs forward in every cardinal direction");
        }

        // A noisy player behind the creature must cause a physical turn before
        // its first step, including large timesteps with several swept samples.
        actor.resetAt(3.75,3.75,heading+PI);
        bool moved=false; int stationaryTurns=0;
        for (int i=0;i<8 && !moved;++i) {
            const double x=actor.x(),z=actor.z(); const float yaw=actor.yaw();
            actor.update(world,player,.1f);
            checkForwardStep(actor,x,z,yaw,.1f);
            check(actor.state()==br::ThreatState::Chase,"heard player behind starts pursuit intent");
            moved=std::hypot(actor.x()-x,actor.z()-z)>1e-7;
            if (!moved) {
                ++stationaryTurns;
                check(actor.x()==3.75 && actor.z()==3.75,"turnaround stays exactly at the original position");
            }
        }
        check(stationaryTurns>=3 && moved,"bounded 180-degree turnaround precedes a forward run");

        actor.resetAt(3.75,3.75,heading+PI/2);
        actor.update(world,player,.1f);
        check(actor.x()==3.75 && actor.z()==3.75 && actor.speed()==0,
              "a 90-degree pursuit turn cannot begin with a sideways step");
        const float quarterTurnYaw=actor.yaw();
        actor.update(world,player,.1f);
        checkForwardStep(actor,3.75,3.75,quarterTurnYaw,.1f);
        check(actor.speed()>3.2f,"a completed 90-degree turn immediately resumes forward running");
    }

    // Quiet patrol uses a slower turn budget even when multiple collision
    // samples are necessary; hiding isolates this from sight/hearing changes.
    br::Threat patrol; patrol.resetAt(3.75,3.75,PI);
    br::ThreatPlayer hidden; hidden.x=hidden.z=3.75; hidden.hidden=true;
    bool walked=false,turned=false;
    for (int i=0;i<70;++i) {
        const double x=patrol.x(),z=patrol.z(); const float yaw=patrol.yaw();
        patrol.update(world,hidden,.1f);
        checkForwardStep(patrol,x,z,yaw,.1f);
        check(patrol.state()==br::ThreatState::Wander && patrol.speed()<1.201f,
              "quiet patrol retains its walking speed and state");
        const float turn=std::abs(std::remainder(patrol.yaw()-yaw,2*PI));
        check(turn<=.60001f,"walking spends at most six radians per second on turning");
        walked|=patrol.speed()>.1f; turned|=turn>.1f;
    }
    check(walked && turned,"patrol fixture exercises both forward walking and turns");

    // The north-wall detour has several grid corners. At a corner even a
    // 100 ms frame may advance only one straight segment, then turn in place.
    br::Threat actor; actor.resetAt(.9,.7,0);
    br::ThreatPlayer player; player.x=.9; player.z=-.7; player.noise=1;
    bool pathUsed=false; int cornerTurns=0,movingFrames=0;
    for (int i=0;i<140 && !actor.caught();++i) {
        const double x=actor.x(),z=actor.z(); const float yaw=actor.yaw();
        actor.update(world,player,.1f);
        checkForwardStep(actor,x,z,yaw,.1f);
        check(!world.blocked(actor.x(),actor.z(),br::Threat::RADIUS),"turning around a corner preserves capsule clearance");
        pathUsed|=actor.pathNodesVisited()>0;
        if (std::hypot(actor.x()-x,actor.z()-z)>1e-7) ++movingFrames;
        else if (movingFrames>0 && std::abs(std::remainder(actor.yaw()-yaw,2*PI))>.1f) ++cornerTurns;
    }
    check(pathUsed && actor.caught() && cornerTurns>=2,
          "nontrivial wall detour visibly pivots at successive corners before catching the player");
    std::cout<<"Forward movement, four directions, bounded turnarounds and corner pivots: PASS\n";
}

void wallAndCornerRecovery() {
    br::World world(42); world.update(3.75,3.75);
    for(float dt:{1.f/60,1.f/30,.1f}) {
        br::ThreatPlayer player;player.x=.9;player.z=.33;player.noise=1;
        check(!world.blocked(player.x,player.z,.23) &&
              world.blocked(player.x,player.z,br::Threat::RADIUS),
              "wall fixture is legal for the player but too tight for the creature");
        br::Threat enemy;enemy.resetAt(3.75,3.75);
        for(int frame=0;frame<int(8/dt)&&!enemy.caught();++frame) {
            const double x=enemy.x(),z=enemy.z();const float yaw=enemy.yaw();
            enemy.update(world,player,dt);checkForwardStep(enemy,x,z,yaw,dt);
            check(!world.blocked(enemy.x(),enemy.z(),br::Threat::RADIUS),
                  "wall-hugging target never pulls the creature into a wall");
        }
        check(enemy.caught(),"creature approaches and catches a wall-hugging player instead of freezing");

        // Previously oscillated forever between z=-4.875/-4.815, alternating
        // Chase/Search and rebuilding its route back to the same grid centre.
        enemy.resetAt(.5,-8);player.x=player.z=3.75;
        bool usedDetour=false;
        for(int frame=0;frame<int(20/dt)&&!enemy.caught();++frame) {
            const double x=enemy.x(),z=enemy.z();const float yaw=enemy.yaw();
            enemy.update(world,player,dt);checkForwardStep(enemy,x,z,yaw,dt);
            usedDetour|=enemy.pathPointCount()>1;
            check(!world.blocked(enemy.x(),enemy.z(),br::Threat::RADIUS),
                  "corner recovery retains swept wall collision");
            check(enemy.pathNodesVisited()<=br::Threat::MAX_PATH_NODES,"recovery search stays bounded");
        }
        check(usedDetour && enemy.caught(),"repeated perception changes cannot trap the actor at a route corner");
    }
    // A route planned around a closed door must shorten when it opens.
    br::World opening(12071998);opening.update(-3.75,1);
    br::DoorInfo door{};check(opening.nearestDoor(-3.75,1,2,door),"dynamic door fixture exists");
    br::Threat enemy;enemy.resetAt(-3.75,-4,PI);
    br::ThreatPlayer target;target.x=-3.75;target.z=1;target.noise=1;
    enemy.update(opening,target,1.f/60);
    check(opening.toggleDoor(door.id,target.x,target.z),"dynamic door opens");
    for(int frame=0;frame<300&&!enemy.caught();++frame) {
        opening.advanceDoors(1.f/60,target.x,target.z);
        target.noise=frame<30?1:0;enemy.update(opening,target,1.f/60);
        check(!opening.blocked(enemy.x(),enemy.z(),br::Threat::RADIUS),"opening-door shortcut respects walls");
    }
    check(enemy.caught(),"retained detour yields to newly opened direct route");
    std::cout<<"Wall-hugging player and corner recovery at 60/30/10 Hz and opening-door shortcut: PASS\n";
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
    check(std::hypot(first.x()-player.x,first.z()-player.z)>=br::Threat::SPAWN_MIN_DISTANCE,"entity appears at least 24 m from player");
    check(std::hypot(first.x()-player.x,first.z()-player.z)<=br::Threat::SPAWN_MAX_DISTANCE+.01,"entity appears within the bounded 32 m spawn band");
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
    check(std::hypot(watched.x()-player.x,watched.z()-player.z)<=br::Threat::SPAWN_MAX_DISTANCE+.01,"turning away permits safe recycling into the spawn band");
    player.forwardX=0; player.forwardZ=-1;

    // Change the streamed working set entirely. The same one actor must be
    // safely recycled into resident space instead of walking through void.
    player.x=903.75; player.z=3.75; world.update(player.x,player.z);
    for(int i=0;i<1200 && std::hypot(first.x()-player.x,first.z()-player.z)>75;++i)
        first.update(world,player,1.f/60);
    check(std::hypot(first.x()-player.x,first.z()-player.z)<=br::Threat::SPAWN_MAX_DISTANCE+.01,"one entity follows distant stream changes into the spawn band");
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

    for (const std::uint64_t seed:{0ULL,1ULL,13ULL,42ULL,91ULL,777ULL,12071998ULL,8675309ULL}) {
        br::World varied(seed); varied.update(3.75,3.75);
        br::Threat actor(seed); br::ThreatPlayer observer; observer.x=observer.z=3.75;
        for(int i=0;i<1800 && !actor.active();++i) actor.update(varied,observer,1.f/60);
        check(actor.active() && !varied.blocked(actor.x(),actor.z(),br::Threat::RADIUS),"safe reachable spawn succeeds across varied seeds");
        const double distance=std::hypot(actor.x()-observer.x,actor.z()-observer.z);
        check(distance>=br::Threat::SPAWN_MIN_DISTANCE && distance<=br::Threat::SPAWN_MAX_DISTANCE,
              "all seeds use the 24 to 32 m spawn band");
        const double forwardDot=((actor.x()-observer.x)*observer.forwardX+(actor.z()-observer.z)*observer.forwardZ)/distance;
        check(forwardDot<.25 || !br::Threat::visibleSegment(varied,observer.x,observer.z,actor.x(),actor.z()),
              "spawn does not pop into the player's view across varied seeds");
        check(actor.pathPointCount()>0 && actor.pathNodesVisited()<=br::Threat::MAX_PATH_NODES,"each seeded spawn has a bounded route");
    }
    std::cout<<"Unreachable path budget and varied-seed reachable spawning: PASS\n";
}

void entranceSpawnClearance() {
    bool entranceCandidateRejected=false;
    bool alarmCandidateRejected=false;
    for (const std::uint64_t seed:{0ULL,1ULL,13ULL,42ULL,91ULL,777ULL,12071998ULL,8675309ULL}) {
        br::World world(seed); world.update(3.75,3.75);
        br::ThreatPlayer entrance; entrance.x=entrance.z=3.75;
        br::ThreatPlayer moved=entrance; moved.x=27.75; moved.forwardX=1; moved.forwardZ=0;
        check(!world.blocked(moved.x,moved.z,br::Threat::RADIUS),"moved player fixture remains in the entrance corridor");
        br::Threat anchored(seed),fresh(seed);
        anchored.update(world,entrance,1.f/60);
        // Moving during the grace period must not allow an appearance next to
        // the original starting location behind the player's current position.
        for(int i=0;i<1800 && !anchored.active();++i) anchored.update(world,moved,1.f/60);
        check(anchored.active(),"a spawn outside both clearance circles remains reachable across seeds");
        const double movedDistance=std::hypot(anchored.x()-moved.x,anchored.z()-moved.z);
        check(movedDistance>=24 && movedDistance<=32,"moving player retains the ordinary 24 to 32 m spawn range");
        check(std::hypot(anchored.x()-entrance.x,anchored.z()-entrance.z)>=24,
              "initial entrance retains 24 m spawn clearance after the player walks away");
        check(anchored.pathPointCount()>0 && anchored.pathNodesVisited()<=br::Threat::MAX_PATH_NODES,
              "entrance clearance preserves a reachable route within the fixed search budget");
        check((anchored.x()-moved.x)/movedDistance<.25 ||
              !br::Threat::visibleSegment(world,moved.x,moved.z,anchored.x(),anchored.z()),
              "entrance-aware spawn is still hidden from the current player view");

        for(int i=0;i<1800 && !fresh.active();++i) fresh.update(world,moved,1.f/60);
        check(fresh.active(),"fresh moved-origin control finds a spawn");
        const bool nearOldEntrance=std::hypot(fresh.x()-entrance.x,fresh.z()-entrance.z)<24;
        entranceCandidateRejected|=nearOldEntrance;
        if (nearOldEntrance) {
            check(anchored.x()!=fresh.x() || anchored.z()!=fresh.z(),"entrance exclusion actually rejects otherwise valid candidates");
            anchored.reset(seed);
            // Paused and zero-length updates must not capture the old origin.
            anchored.update(world,entrance,.1f,true);
            anchored.update(world,entrance,0);
            for(int i=0;i<1800 && !anchored.active();++i) anchored.update(world,moved,1.f/60);
            check(anchored.active() && anchored.x()==fresh.x() && anchored.z()==fresh.z(),
                  "reset clears the old anchor and the first ordinary update records the new origin");
        }
        anchored.resetAt(moved.x-1,moved.z,PI/2);
        check(anchored.active() && anchored.x()==moved.x-1 && anchored.z()==moved.z,
              "explicit capture fixtures can still reset inside the ordinary spawn clearance");

        // An alarm may arrive before update(). With no resident geometry its
        // first attempt fails, but later retries must retain that first origin.
        br::World alarmWorld(seed);
        br::Threat earlyAlarm(seed),alarmControl(seed);
        earlyAlarm.hearAlarm(alarmWorld,entrance,entrance.x,entrance.z,true);
        alarmControl.hearAlarm(alarmWorld,moved,moved.x,moved.z,true);
        check(!earlyAlarm.active() && !alarmControl.active(),"pre-update alarm cannot spawn into nonresident geometry");
        alarmWorld.update(moved.x,moved.z);
        for(int i=0;i<1800 && !earlyAlarm.active();++i) earlyAlarm.update(alarmWorld,moved,1.f/60);
        for(int i=0;i<1800 && !alarmControl.active();++i) alarmControl.update(alarmWorld,moved,1.f/60);
        check(earlyAlarm.active() && alarmControl.active(),"pre-update alarm retries can find reachable loaded space");
        const double alarmDistance=std::hypot(earlyAlarm.x()-moved.x,earlyAlarm.z()-moved.z);
        check(alarmDistance>=24 && alarmDistance<=32 &&
              std::hypot(earlyAlarm.x()-entrance.x,earlyAlarm.z()-entrance.z)>=24,
              "pre-update alarm preserves both the player range and the original entrance clearance");
        check(earlyAlarm.pathPointCount()>0 && earlyAlarm.pathNodesVisited()<=br::Threat::MAX_PATH_NODES,
              "alarm spawn still proves a route within the fixed search budget");
        check((earlyAlarm.x()-moved.x)/alarmDistance<.25 ||
              !br::Threat::visibleSegment(alarmWorld,moved.x,moved.z,earlyAlarm.x(),earlyAlarm.z()),
              "pre-update alarm spawn remains hidden");
        alarmCandidateRejected|=std::hypot(alarmControl.x()-entrance.x,alarmControl.z()-entrance.z)<24;
    }
    check(entranceCandidateRejected,"seed sweep includes candidates that would otherwise spawn too near the original entrance");
    check(alarmCandidateRejected,"pre-update alarm sweep exercises rejection near its original entrance");
    std::cout<<"Entrance clearance after moving away, reset anchor, early alarm and explicit fixtures: PASS\n";
}
}

int main() {
    try {
        forwardLocomotion(); wallAndCornerRecovery(); perceptionAndCatch(); occlusionAndDoor(); memoryPauseAndNavigation(); safeSpawnAndStreaming(); unreachableBudgetAndSeedSweep(); entranceSpawnClearance();
        std::cout<<"Threat tests: PASS\n"; return 0;
    } catch (const std::exception& error) {
        std::cerr<<"Threat tests: FAIL: "<<error.what()<<'\n'; return 1;
    }
}
