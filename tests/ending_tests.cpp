#include "common/ending.h"
#include "common/ending_city.h"
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace br;
static void check(bool yes,const char* message){if(!yes)throw std::runtime_error(message);}
static bool same(const CityActorPose& a,const CityActorPose& b) {
    return a.position.x==b.position.x&&a.position.y==b.position.y&&a.position.z==b.position.z&&
           a.yaw==b.yaw&&a.gait==b.gait&&a.speed==b.speed;
}
static bool finite(const CityActorPose& a) {
    return std::isfinite(a.position.x)&&std::isfinite(a.position.y)&&std::isfinite(a.position.z)&&
           std::isfinite(a.yaw)&&std::isfinite(a.gait)&&std::isfinite(a.speed);
}
static void timeline() {
    EndingSequence sequence;
    sequence.update(.1f);check(sequence.time()==0&&!sequence.finished(),"inactive sequence must not run");
    sequence.start();sequence.update(.1f);const float before=sequence.time();
    sequence.update(.1f,true);check(sequence.time()==before,"pause freezes sequence");
    sequence.update(std::numeric_limits<float>::infinity());sequence.update(-1);sequence.update(0);
    check(sequence.time()==before,"invalid time steps cannot corrupt sequence");
    sequence.update(100);check(std::abs(sequence.time()-before-.25f)<1e-5,"stall step is bounded");
    sequence.start(-9);check(sequence.time()==0,"negative preview is clamped");
    sequence.start(100);check(sequence.finished()&&sequence.time()==ENDING_DURATION,"preview at end is complete");
    sequence.reset();check(sequence.time()==0&&!sequence.finished(),"reset releases completed sequence");
    for(int hz:{8,30,60,120}) {
        sequence.start();for(int i=0;i<43*hz;++i)sequence.update(1.f/hz);
        check(sequence.finished()&&sequence.time()==ENDING_DURATION,"duration independent of frame rate");
    }
    check(std::strcmp(EndingSequence::phaseName(26),"City street")==0,"exterior departure is identified");
    check(std::strcmp(EndingSequence::phaseName(32),"City life")==0,"city is observed after departure");
    check(std::strcmp(EndingSequence::phaseName(40),"Fade")==0,"city remains until closing fade");
    check(sampleEndingPose(0).elevatorOpen==1,"cabin begins open");
    check(sampleEndingPose(4).elevatorOpen==0&&sampleEndingPose(9).elevatorOpen==0,"ascent remains behind closed doors");
    check(!sampleEndingPose(4.99f).surface&&sampleEndingPose(5).surface,"set changes only during ascent");
    check(sampleEndingPose(13).elevatorOpen==1,"cabin doors open completely before departure");
    check(sampleEndingPose(20).exitOpen==0&&sampleEndingPose(23.2f).exitOpen==1,"outer door genuinely animates");
    check(sampleEndingPose(0).fade==1&&sampleEndingPose(1).fade==0&&sampleEndingPose(39).fade==0&&sampleEndingPose(42).fade==1,"fade endpoints");
    auto prev=sampleEndingPose(0);
    for(int i=1;i<=42000;++i) {
        const auto p=sampleEndingPose(i*.001f);
        check(std::isfinite(p.eye.x)&&std::isfinite(p.eye.y)&&std::isfinite(p.eye.z)&&std::isfinite(p.fov),"pose remains finite");
        check(std::abs(dot(p.forward,p.forward)-1)<1e-4,"camera direction remains normalized");
        check(p.elevatorOpen>=0&&p.elevatorOpen<=1&&p.exitOpen>=0&&p.exitOpen<=1,"door animation remains bounded");
        check(std::sqrt(dot(p.eye-prev.eye,p.eye-prev.eye))<.004f,"continuous camera movement without teleports");
        check(p.eye.y>1.58f&&p.eye.y<1.66f,"stable eye height");
        if(p.eye.z<-1.4f)check(p.elevatorOpen>.99f,"camera cannot cross closed lift doors");
        if(p.eye.z<-10.3f)check(p.exitOpen>.99f,"camera cannot cross closed exit door");
        if(p.eye.z>-11.1f)check(std::abs(p.eye.x)<.3f,"lateral walk waits until the exit jamb is cleared");
        check(p.eye.z>=-14.21f,"camera remains on near sidewalk, outside traffic and pedestrian lanes");
        check(p.lobbyLight==1,"normal surface lobby stays lit");
        prev=p;
    }
    const auto outside=sampleEndingPose(30),last=sampleEndingPose(39);
    check(std::abs(outside.eye.x-1.4f)<1e-5&&std::abs(outside.eye.z+14.2f)<1e-5,"camera actually exits to the sidewalk");
    check(last.forward.x>.5f&&last.forward.z<-.7f,"camera pans to observe the street");
    check(last.eye.x==outside.eye.x&&last.eye.z==outside.eye.z,"observation shot is stationary and does not drift into traffic");
}
static void city() {
    constexpr float dt=.02f;
    for(int frame=0;frame<2100;++frame) {
        const float time=frame*dt;
        const auto camera=sampleEndingPose(time);
        for(int i=0;i<EndingCity::pedestrianCount;++i) {
            const auto a=EndingCity::pedestrian(i,time),b=EndingCity::pedestrian(i,time+dt);
            check(finite(a)&&a.speed>=1&&a.speed<=1.5f,"pedestrian speed and pose are finite");
            check(a.position.y==0&&a.position.x>=-90&&a.position.x<=90,"pedestrians stay grounded and bounded");
            check((i<6&&a.position.z<=-15.6f&&a.position.z>=-16.2f)||
                  (i>=6&&a.position.z<=-28.8f&&a.position.z>=-29.6f),"people remain on the two sidewalks");
            const float dx=b.position.x-a.position.x;
            if(std::abs(dx)<1) {
                check(std::abs(std::abs(dx)/dt-a.speed)<.0015f,"walking translates at physical speed");
                check(dx*(-std::sin(a.yaw))>0,"people face their actual walking direction");
            }
            check(std::hypot(a.position.x-camera.eye.x,a.position.z-camera.eye.z)>1.25f,"camera does not collide with pedestrians");
            check(a.gait>=0&&a.gait<=2*PI,"gait angle remains numerically bounded");
            check(same(a,EndingCity::pedestrian(i,time)),"same paused time freezes all walking parts exactly");
        }
        for(int i=0;i<EndingCity::carCount;++i) {
            const auto a=EndingCity::car(i,time),b=EndingCity::car(i,time+dt);
            check(finite(a)&&a.speed>=5&&a.speed<=7,"traffic speed and pose are finite");
            check(a.position.y==-.15f&&a.position.x>=-90&&a.position.x<=90,"cars rest on the road and remain bounded");
            check(a.position.z==(i<3?-20.f:-24.f),"traffic stays in its assigned lane");
            const float dx=b.position.x-a.position.x;
            if(std::abs(dx)<1) {
                check(std::abs(std::abs(dx)/dt-a.speed)<.0015f,"cars move at their rolling speed");
                check(dx*(-std::sin(a.yaw))>0,"cars face their driving direction");
            }
            for(int j=i+1;j<EndingCity::carCount;++j) {
                const auto other=EndingCity::car(j,time);
                if(a.position.z==other.position.z) {
                    const float distance=std::abs(a.position.x-other.position.x);
                    check(std::min(distance,180-distance)>30,"same-lane cars cannot overlap, including wrap boundary");
                }
            }
            check(same(a,EndingCity::car(i,time)),"same paused time freezes traffic exactly");
        }
    }
    for(float time:{-1.f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),1e30f})
        for(int i=0;i<12;++i)check(finite(EndingCity::pedestrian(i,time))&&finite(EndingCity::car(i,time)),"invalid and huge samples stay finite");
    check(same(EndingCity::car(0,-1),EndingCity::car(0,0)),"negative scene time clamps to start");
    check(same(EndingCity::pedestrian(0,-1),EndingCity::pedestrian(0,0)),"negative pedestrian time clamps to start");
    // There must be street activity in the doorway view and later city shots.
    for(float time:{26.f,32.f,37.f}) {
        int walkers=0,cars=0;
        for(int i=0;i<12;++i)if(std::abs(EndingCity::pedestrian(i,time).position.x)<20)++walkers;
        for(int i=0;i<6;++i)if(std::abs(EndingCity::car(i,time).position.x)<20)++cars;
        check(walkers>=4&&cars>=1,"central street has moving people and cars at each city milestone");
    }
}
int main(){try{
    timeline();city();
    std::cout<<"Ending tests passed: 42s timeline, pause/replay, door/camera clearance, continuous city departure, sidewalk walking, traffic spacing/orientation and deterministic bounded animation.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
