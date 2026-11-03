#include "../common/presence.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr float DT=.05f;
constexpr std::uint64_t SEED=12071998;
void check(bool value,const std::string& message){if(!value)throw std::runtime_error(message);}

struct Fixture { br::DoorInfo door; br::PresenceContext state; };
bool clear(const br::World& world,br::WorldPoint a,br::WorldPoint b) {
    const int n=std::max(1,int(std::ceil(std::hypot(b.x-a.x,b.z-a.z)/.1)));
    for(int i=0;i<=n;++i)if(world.blocked(a.x+(b.x-a.x)*i/n,a.z+(b.z-a.z)*i/n,.06))return false;
    return true;
}

Fixture findFixture(const br::World& world) {
    std::vector<br::DoorInfo> doors;
    for(const auto& entry:world.chunks())for(const auto& door:entry.second.doors)
        if(!door.isExit&&!door.open)doors.push_back(door);
    std::sort(doors.begin(),doors.end(),[](const auto& a,const auto& b){
        return std::hypot(a.x-3.75,a.z-3.75)<std::hypot(b.x-3.75,b.z-3.75);
    });
    for(const auto& door:doors)for(double side:{-1.,1.})for(double distance:{5.5,6.5,8.}) {
        br::PresenceContext state;
        state.threatActive=state.threatWandering=true;
        state.listener={door.x+(door.east?side*distance:0),door.z+(door.east?0:side*distance)};
        state.threat={door.x-(door.east?side*3.75:0),door.z-(door.east?0:side*3.75)};
        state.forwardX=float((door.x-state.listener.x)/distance);
        state.forwardZ=float((door.z-state.listener.z)/distance);
        const br::WorldPoint near{door.x+(door.east?side*.35:0),door.z+(door.east?0:side*.35)};
        const br::WorldPoint far{door.x-(door.east?side*.35:0),door.z-(door.east?0:side*.35)};
        if(!world.blocked(state.listener.x,state.listener.z,.23)&&
           !world.blocked(state.threat.x,state.threat.z,.26)&&
           clear(world,state.listener,near)&&clear(world,state.threat,far)&&
           world.soundTransmission(state.listener.x,state.listener.z,state.threat.x,state.threat.z)<=.5f)
            return {door,state};
    }
    throw std::runtime_error("No actual closed-door fixture found");
}

double first(br::Level0Presence& presence,const br::World& world,const br::PresenceContext& state,
             br::DoorRattleCue* found=nullptr,double seconds=70) {
    for(int i=0;i<int(seconds/DT);++i) {
        const auto cue=presence.update(world,state,DT);
        check(presence.candidateChecks()<=br::Level0Presence::MAX_CANDIDATES,"candidate ray budget is bounded");
        if(cue){if(found)*found=*cue;return (i+1)*double(DT);}
    }
    return -1;
}

void isolateDoor(br::World& world,const Fixture& fixture) {
    std::vector<br::DoorInfo> nearby;
    for(const auto& entry:world.chunks())for(const auto& door:entry.second.doors)
        if(!door.isExit&&!door.open&&door.id!=fixture.door.id&&
           std::hypot(door.x-fixture.state.threat.x,door.z-fixture.state.threat.z)<=10)nearby.push_back(door);
    for(const auto& door:nearby)check(world.toggleDoor(door.id,door.x+(door.east?.75:0),door.z+(door.east?0:.75),.23),
                                    "open other nearby leaves for isolated fixture");
    for(int i=0;i<10;++i)world.advanceDoors(.1f,10000,10000,.23);
}

bool sameVertex(const br::Vertex& a,const br::Vertex& b) {
    return a.x==b.x&&a.y==b.y&&a.z==b.z&&a.nx==b.nx&&a.ny==b.ny&&a.nz==b.nz&&
           a.u==b.u&&a.v==b.v&&a.material==b.material;
}
bool sameMesh(const std::vector<br::Vertex>& a,const std::vector<br::Vertex>& b) {
    return a.size()==b.size()&&std::equal(a.begin(),a.end(),b.begin(),sameVertex);
}

void handleGeometry(const br::World& world,const Fixture& fixture) {
    const auto original=world.doorVertices(0,0);
    std::vector<bool> collisions;
    for(int z=-8;z<=8;++z)for(int x=-8;x<=8;++x)
        collisions.push_back(world.blocked(fixture.door.x+x*.15,fixture.door.z+z*.15,.23));
    const auto remembered=world.rememberedDoors();
    for(float angle:{-.3f,.3f}) {
        const auto moved=world.doorVertices(0,0,fixture.door.id,angle);
        check(original.size()==moved.size(),"visual override preserves mesh size");
        unsigned changed=0;
        for(std::size_t i=0;i<original.size();++i)if(!sameVertex(original[i],moved[i])) {
            ++changed;
            const auto& v=moved[i];
            check(v.material==7&&original[i].material==7,"only metal handle vertices change");
            check(std::abs(v.nx*v.nx+v.ny*v.ny+v.nz*v.nz-1)<.00001,
                  "rotated handle normals remain unit length");
            check(std::hypot(v.x-original[i].x,v.y-original[i].y,v.z-original[i].z)<.04,
                  "handle movement remains small without leaf movement");
        }
        check(changed==36,"exactly one six-face handle moves; all leaves and other handles are identical");
    }
    check(sameMesh(original,world.doorVertices(0,0,fixture.door.id,0)),"zero angle preserves original mesh exactly");
    check(sameMesh(original,world.doorVertices(0,0,std::nullopt,.3f)),"missing target leaves original mesh intact");
    check(sameMesh(original,world.doorVertices(0,0,fixture.door.id,std::numeric_limits<float>::quiet_NaN())),
          "nonfinite visual rotation cannot poison geometry");
    check(sameMesh(world.doorVertices(0,0,fixture.door.id,2),world.doorVertices(0,0,fixture.door.id,.3f)),
          "renderer independently clamps untrusted angle");
    check(sameMesh(original,world.doorVertices(0,0)),"visual override has no persistent mesh state");
    check(world.rememberedDoors()==remembered,"visual mesh construction cannot mutate door memory");
    std::size_t index=0;
    for(int z=-8;z<=8;++z)for(int x=-8;x<=8;++x)
        check(collisions[index++]==world.blocked(fixture.door.x+x*.15,fixture.door.z+z*.15,.23),
              "collision capsule results remain identical throughout handle presentation");
    br::DoorInfo actual;
    check(world.nearestDoor(fixture.door.x,fixture.door.z,.1,actual)&&actual.open==fixture.door.open&&
          actual.openness==fixture.door.openness&&actual.swing==fixture.door.swing,
          "door pose and requested state are unchanged");
    for(auto level:{br::Level::Zero,br::Level::Fun,br::Level::Parking}) {
        br::World exits(SEED,level);const auto location=exits.exitLocation();exits.update(location.x,location.z,1);
        bool tested=false;
        for(const auto& entry:exits.chunks())for(const auto& door:entry.second.doors)if(door.isExit) {
            check(sameMesh(exits.doorVertices(0,0),exits.doorVertices(0,0,door.id,.3f)),
                  "exit handles in all three levels ignore decorative override");
            tested=true;
        }
        check(tested,"actual exit mesh is loaded for every level isolation check");
    }
}

void cadenceAndState(br::World& world,const Fixture& fixture) {
    br::Level0Presence presence(SEED),same(SEED);
    const auto remembered=world.rememberedDoors();
    br::DoorRattleCue cue{};
    const auto initial=first(presence,world,fixture.state,&cue,22);
    check(initial>=12&&initial<=20.1,"first rattle waits12–20 active eligible seconds");
    check(cue.doorId==fixture.door.id&&presence.doorId()==cue.doorId,"only the selected real door is animated");
    check(std::hypot(cue.position.x-fixture.door.x,cue.position.z-fixture.door.z)<1,"cue is attached to its handle");
    const auto repeated=first(same,world,fixture.state,nullptr,22);
    check(std::abs(initial-repeated)<.0001,"seeded timing is deterministic");

    float previous=0,maxAngle=0;
    int movements=0;
    for(int frame=0;frame<20;++frame) {
        check(!presence.update(world,fixture.state,DT),"an active animation never emits another start cue");
        const float angle=presence.handleAngle();
        maxAngle=std::max(maxAngle,std::abs(angle));
        if(previous==0&&std::abs(angle)>.001)++movements;
        previous=std::abs(angle)<.001?0:angle;
        check(std::isfinite(angle)&&std::abs(angle)<=.30001f,"handle rotation is bounded");
    }
    check(movements==2&&maxAngle>.25f,"two visible pulls use the intended envelope");
    check(!presence.active()&&presence.handleAngle()==0,"animation settles and releases its door");
    check(world.rememberedDoors()==remembered,"presentation cannot mutate door overrides");
    br::DoorInfo actual;
    check(world.nearestDoor(fixture.door.x,fixture.door.z,.1,actual)&&!actual.open&&actual.openness==0,
          "leaf stays closed during visual hint");
    const double next=first(presence,world,fixture.state,nullptr,66)+20*double(DT);
    check(next>=40&&next<=65.1,"repeat cooldown is40–65 seconds from previous start");
    std::cout<<"Fixture: door="<<fixture.door.id<<" center="<<fixture.door.x<<","<<fixture.door.z
             <<" east="<<fixture.door.east<<" player="<<fixture.state.listener.x<<","<<fixture.state.listener.z
             <<" threat="<<fixture.state.threat.x<<","<<fixture.state.threat.z
             <<" forward="<<fixture.state.forwardX<<","<<fixture.state.forwardZ
             <<" first="<<initial<<"s\n";
}

void pauseResetAndSafety(const br::World& world,const Fixture& fixture) {
    br::Level0Presence paused(SEED),reference(SEED);
    auto state=fixture.state;state.paused=true;
    check(first(paused,world,state,nullptr,80)<0,"pause emits no sound cue");
    check(first(paused,world,fixture.state)==first(reference,world,fixture.state),"pause freezes initial cooldown");
    paused.update(world,fixture.state,.1f);
    const auto id=paused.doorId();const float angle=paused.handleAngle();
    for(int i=0;i<200;++i)check(!paused.update(world,state,.1f),"paused active event stays silent");
    check(paused.doorId()==id&&paused.handleAngle()==angle,"pause freezes animation pose exactly");
    paused.reset(SEED);reference.reset(SEED);
    check(!paused.active()&&paused.handleAngle()==0,"reset releases old animation");
    check(first(paused,world,fixture.state)==first(reference,world,fixture.state),"reset restores seeded schedule");

    for(int variant=0;variant<8;++variant) {
        auto invalid=fixture.state;
        if(variant==0)invalid.threatActive=false;
        if(variant==1)invalid.threatWandering=false;
        if(variant==2)invalid.hidden=true;
        if(variant==3)invalid.alive=false;
        if(variant==4)invalid.threat.x=std::numeric_limits<double>::quiet_NaN();
        if(variant==5)invalid.listener.z=std::numeric_limits<double>::infinity();
        if(variant==6)invalid.forwardX=std::numeric_limits<float>::infinity();
        if(variant==7)invalid.forwardX=invalid.forwardZ=0;
        br::Level0Presence p(SEED);
        check(first(p,world,invalid,nullptr,70)<0,"invalid/absent/dangerous context cannot manufacture a hint");
    }
    br::Level0Presence clamped(SEED);
    for(int i=0;i<100;++i)check(!clamped.update(world,fixture.state,1000),"long frame is capped to .1 seconds");
    check(!clamped.update(world,fixture.state,-1)&&
          !clamped.update(world,fixture.state,std::numeric_limits<float>::quiet_NaN()),"invalid dt cannot advance");
}

void geometryAndCancellation(br::World& world,const Fixture& fixture) {
    auto sameSide=fixture.state;
    sameSide.threat={fixture.door.x+(fixture.door.east?sameSide.forwardX*-2:0),
                    fixture.door.z+(fixture.door.east?0:sameSide.forwardZ*-2)};
    br::Level0Presence same(SEED);
    check(first(same,world,sameSide,nullptr,70)<0,"same-side actor cannot rattle a hidden far-side handle");
    auto near=fixture.state;near.threat=near.listener;
    br::Level0Presence close(SEED);
    check(first(close,world,near,nullptr,70)<0,"close creature suppresses decorative event");
    auto distant=fixture.state;distant.threat.x+=100;
    br::Level0Presence far(SEED);
    check(first(far,world,distant,nullptr,70)<0,"distant creature cannot affect a nearby door");
    auto lookingAway=fixture.state;lookingAway.forwardX*=-1;lookingAway.forwardZ*=-1;
    br::Level0Presence away(SEED);
    check(first(away,world,lookingAway,nullptr,70)<0,"door outside player's gaze is not selected");

    // A walkable viewpoint separated from the same door by another real wall.
    auto occluded=fixture.state;bool found=false;
    const double side=(fixture.door.east?fixture.state.listener.x-fixture.door.x:
                       fixture.state.listener.z-fixture.door.z)>0?1:-1;
    const br::WorldPoint nearApproach{fixture.door.x+(fixture.door.east?.35*side:0),
                                     fixture.door.z+(fixture.door.east?0:.35*side)};
    for(double distance:{6.,9.,11.})for(int degree=0;degree<360&&!found;degree+=10) {
        const double a=degree*3.14159265358979323846/180;
        const br::WorldPoint p{fixture.door.x+std::cos(a)*distance,fixture.door.z+std::sin(a)*distance};
        const double playerSide=fixture.door.east?p.x-fixture.door.x:p.z-fixture.door.z;
        if(playerSide*side<=.1||world.blocked(p.x,p.z,.23)||
           std::hypot(p.x-fixture.state.threat.x,p.z-fixture.state.threat.z)<7||clear(world,p,nearApproach))continue;
        occluded.listener=p;occluded.forwardX=float((fixture.door.x-p.x)/distance);
        occluded.forwardZ=float((fixture.door.z-p.z)/distance);found=true;
    }
    check(found,"find a genuinely occluded near-side viewpoint");
    br::Level0Presence blocked(SEED);int previousProbe=-100;unsigned probes=0;
    for(int i=0;i<600;++i) {
        check(!blocked.update(world,occluded,DT),"occluded door is never animated through walls");
        if(blocked.candidateChecks()) {
            check(blocked.candidateChecks()<=8,"at most eight geometry candidates per scan");
            check(i-previousProbe>=19,"failed geometry placement retries at most once per second");
            previousProbe=i;++probes;
        }
    }
    check(probes>1,"blocked fixture exercises actual bounded geometry retries");

    auto checkCancel=[&](br::PresenceContext invalid,const char* why) {
        br::Level0Presence p(SEED);check(first(p,world,fixture.state)>0,"prime cancellation fixture");
        p.update(world,invalid,DT);check(!p.active()&&p.handleAngle()==0,why);
    };
    auto danger=fixture.state;danger.threatWandering=false;checkCancel(danger,"chase/search cancels active hint");
    auto dead=fixture.state;dead.alive=false;checkCancel(dead,"death cancels active hint");
    auto hidden=fixture.state;hidden.hidden=true;checkCancel(hidden,"hiding cancels active hint");
    checkCancel(distant,"actor leaving door cancels active hint");
    auto left=fixture.state;left.listener.x+=50;checkCancel(left,"player leaving range cancels active hint");

    br::Level0Presence opened(SEED);check(first(opened,world,fixture.state)>0,"prime opened door fixture");
    const double interactX=fixture.door.x+(fixture.door.east?.75:0);
    const double interactZ=fixture.door.z+(fixture.door.east?0:.75);
    check(world.toggleDoor(fixture.door.id,interactX,interactZ),"open active door normally");
    check(sameMesh(world.doorVertices(0,0),world.doorVertices(0,0,fixture.door.id,.3f)),
          "opening leaf immediately refuses decorative handle override");
    opened.update(world,fixture.state,DT);
    check(!opened.active(),"request to open the leaf cancels its handle hint immediately");
    br::Level0Presence alreadyOpen(SEED);
    check(first(alreadyOpen,world,fixture.state,nullptr,30)<0,"open door cannot start another hint");
    check(world.toggleDoor(fixture.door.id,interactX,interactZ),"restore closed fixture");
    world.advanceDoors(.1f,fixture.state.listener.x,fixture.state.listener.z);

    for(auto level:{br::Level::Fun,br::Level::Parking}) {
        br::World other(SEED,level);other.update(3.75,3.75,0);
        br::Level0Presence p(SEED);check(first(p,world,fixture.state)>0,"prime level-change fixture");
        p.update(other,fixture.state,DT);check(!p.active(),"other levels clear Level0 presentation");
        check(first(p,other,fixture.state,nullptr,30)<0,"other levels never emit Level0 cues");
    }
    br::Level0Presence unloaded(SEED);check(first(unloaded,world,fixture.state)>0,"prime unload fixture");
    world.update(1000,1000,0);unloaded.update(world,fixture.state,DT);
    check(!unloaded.active(),"unloading target door cancels presentation");
}
}

int main() {
    try {
        br::World world(SEED);world.update(3.75,3.75);
        const auto fixture=findFixture(world);isolateDoor(world,fixture);
        handleGeometry(world,fixture);
        cadenceAndState(world,fixture);
        pauseResetAndSafety(world,fixture);
        geometryAndCancellation(world,fixture);
        std::cout<<"Presence tests passed.\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<"Presence test failure: "<<error.what()<<'\n';return 1;}
}
