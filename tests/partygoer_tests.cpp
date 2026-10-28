#include "common/partygoer_model.h"
#include <iostream>
#include <stdexcept>
#include <set>
using namespace br;
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try {
    World world(12071998,Level::Fun);world.update(3.75,3.75,1);
    Partygoers group;auto& actor=group.actors[0];
    actor.active=true;actor.x=3.75;actor.z=.75;actor.yaw=PI;
    const Vec3 toward{0,0,-1},away{0,0,1};
    Partygoers cadence;cadence.actors[0]=actor;
    float previousPhase=0;int footfalls=0;double distance=0;
    for(int i=0;i<60;++i) {
        const auto before=cadence.actors[0];
        cadence.update(world,3.75,3.75,1.65f,away,78,1.6f,1.f/60,false);
        const auto& after=cadence.actors[0];
        distance+=std::hypot(after.x-before.x,after.z-before.z);
        if(after.phase>previousPhase && (previousPhase==0 || int(after.phase/PI)>int(previousPhase/PI)))++footfalls;
        previousPhase=after.phase;
    }
    check(distance>1 && footfalls>=1 && footfalls<=3,"one moving Partygoer produces separate strides rather than crowd-like rapid taps");

    check(Partygoers::seen(world,actor,3.75,3.75,1.65f,toward,78,1.6f),"head in view freezes");
    check(!Partygoers::seen(world,actor,3.75,3.75,1.65f,away,78,1.6f),"behind camera is not seen");
    check(!Partygoers::seen(world,actor,3.75,3.75,1.65f,{0,-.999f,-.04f},78,1.6f),"vertical field of view matters");
    auto initial=actor;
    for(int i=0;i<90;++i)group.update(world,3.75,3.75,1.65f,toward,78,1.6f,1.f/60,false);
    check(actor.x==initial.x && actor.z==initial.z && actor.yaw==initial.yaw && actor.phase==initial.phase,"look freezes position, rotation and animation exactly");
    check(group.activeCount()==4,"four independently spawned actors");
    check(group.travelled>0,"unseen actors can move while the observed one stays frozen");
    for(int i=0;i<90;++i)group.update(world,3.75,3.75,1.65f,away,78,1.6f,1.f/60,false);
    check(actor.z>initial.z+.5,"looking away resumes approach");
    check(!world.blocked(actor.x,actor.z,Partygoers::RADIUS),"approach respects furniture and walls");
    auto stopped=actor;
    for(int i=0;i<60;++i)group.update(world,3.75,3.75,1.65f,away,78,1.6f,1.f/60,true);
    check(actor.z==stopped.z && actor.phase==stopped.phase,"pause suspends AI and animation");
    for(int i=0;i<240;++i)group.update(world,3.75,3.75,1.65f,away,78,1.6f,1.f/60,false);
    check(group.touchingPlayer(world,3.75,3.75),"Partygoer reaches lethal contact distance");
    Partygoers touching;touching.actors[0].active=true;touching.actors[0].x=3.75;touching.actors[0].z=3.2;
    check(touching.touchingPlayer(world,3.75,3.75),"walking into a frozen actor is lethal too");
    bool testedWall=false;
    for(const auto& [coord,c]:world.chunks())for(const auto& b:c.sightBlockers) {
        if(testedWall || b.bottom>0 || b.top<2)continue;
        double x=(b.footprint.minX+b.footprint.maxX)*.5+coord.x*CHUNK_SIZE,z=(b.footprint.minZ+b.footprint.maxZ)*.5+coord.z*CHUNK_SIZE;
        bool east=b.footprint.maxX-b.footprint.minX < b.footprint.maxZ-b.footprint.minZ;
        check(!Partygoers::clearSight(world,x-(east?.7:0),1.8,z-(east?0:.7),x+(east?.7:0),1.8,z+(east?0:.7)),"opaque wall blocks gaze");
        touching.actors[0].x=x+(east?.29:0);touching.actors[0].z=z+(east?0:.29);
        check(!touching.touchingPlayer(world,x-(east?.29:0),z-(east?0:.29)),"contact cannot kill through a wall");
        Partygoers::Actor hidden;hidden.active=true;hidden.x=x+(east?.36:0);hidden.z=z+(east?0:.36);
        Vec3 view=east?Vec3{1,0,0}:Vec3{0,0,1};
        check(!Partygoers::seen(world,hidden,x-(east?1:0),z-(east?0:1),1.65f,view,94,1.6f),"body behind wall is not frozen by protruding gaze samples");
        testedWall=true;
    }
    check(testedWall,"wall visibility fixture exists");
    const auto mesh=partygoerVertices(group,0,0);std::set<int> materials;
    check(mesh.size()>0 && mesh.size()/3<6000,"bounded procedural population geometry");
    for(const auto& v:mesh){check(std::isfinite(v.x)&&std::isfinite(v.nx),"finite model");materials.insert(int(v.material));}
    for(int material:{48,55,56})check(materials.count(material),"balloon, yellow costume and painted face");
    Partygoers normal,boosted;
    for(auto* g:{&normal,&boosted}){auto& a=g->actors[0];a.active=true;a.x=3.75;a.z=.75;a.yaw=PI;}
    boosted.hearAlarm(3.75,3.75);
    for(int i=0;i<20;++i) {
        normal.update(world,3.75,3.75,1.65f,away,78,1.6f,1.f/60,false);
        boosted.update(world,3.75,3.75,1.65f,away,78,1.6f,1.f/60,false);
    }
    check(boosted.actors[0].z>normal.actors[0].z+.25,"mouse alert increases movement speed");
    group.hearAlarm(3.75,.75);
    check(group.alarmRemaining==6 && group.alarmTarget.z==.75,"mouse alarm records one shared destination");
    for(const auto& a:group.actors)check(a.repath==0 && a.path.empty(),"alarm invalidates every Partygoer route");
    group.update(world,3.75,3.75,1.65f,toward,78,1.6f,.05f,true);
    check(group.alarmRemaining==6,"pause freezes alarm memory");
    group.actors[0].x=3.75;group.actors[0].z=.75;
    const auto frozen=group.actors[0];
    group.update(world,3.75,3.75,1.65f,toward,78,1.6f,.05f,false);
    check(group.actors[0].x==frozen.x && group.actors[0].z==frozen.z,"alarm never overrides gaze freeze");
    for(int i=0;i<130;++i)group.update(world,3.75,3.75,1.65f,toward,78,1.6f,.05f,false);
    check(group.alarmRemaining==0,"alarm expires without repeated mouse calls");
    World zero(42);zero.update(3.75,3.75,0);group.update(zero,3.75,3.75,1.65f,toward,78,1.6f,.016f,false);
    check(group.activeCount()==0,"Level 0 isolation");
    group.reset();check(group.activeCount()==0 && group.travelled==0,"restart clears population");
    std::cout<<"Partygoer tests passed: independent gaze, walls, pitch, freeze/resume, pause, lethal contact and wall rejection, population, collision, bounded mesh and reset.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
