#include "common/ending.h"
#include "common/ending_city.h"
#include "common/ending_city_feet.h"
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace br;
static void check(bool yes,const char* message){if(!yes)throw std::runtime_error(message);}
static bool same(const CityActorPose& a,const CityActorPose& b) {
    return a.position.x==b.position.x&&a.position.y==b.position.y&&a.position.z==b.position.z&&
           a.yaw==b.yaw&&a.gait==b.gait&&a.speed==b.speed&&a.headYaw==b.headYaw&&a.brake==b.brake;
}
static bool finite(const CityActorPose& a) {
    return std::isfinite(a.position.x)&&std::isfinite(a.position.y)&&std::isfinite(a.position.z)&&
           std::isfinite(a.yaw)&&std::isfinite(a.gait)&&std::isfinite(a.speed)&&std::isfinite(a.headYaw)&&std::isfinite(a.brake);
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
static float cyclicDistance(float a,float b) {
    const float d=std::abs(a-b);return std::min(d,180-d);
}
static void city() {
    constexpr float dt=.02f;
    for(int frame=0;frame<2100;++frame) {
        const float time=frame*dt;
        const auto camera=sampleEndingPose(time);
        for(int i=0;i<EndingCity::pedestrianCount;++i) {
            const auto a=EndingCity::pedestrian(i,time),b=EndingCity::pedestrian(i,time+dt);
            check(finite(a)&&a.speed>=1&&a.speed<=1.5f,"pedestrian speed and pose are finite");
            check(a.position.x>=-90&&a.position.x<=90,"pedestrians stay bounded");
            if(i==EndingCity::crossingPedestrian) {
                check(a.position.y>=-.151f&&a.position.y<=.036f,"crossing body follows bounded pavement and road height");
                if(a.position.z>-26.25f&&a.position.z<-17.8f)
                    check(std::abs(a.position.y+.15f)<1e-5,"crossing feet are rooted at road height, not sidewalk height");
                check(std::abs(b.position.y-a.position.y)<.025f,"curb step height changes smoothly");
            } else check(a.position.y==0,"ordinary walkers stay on pavement");
            if(i!=EndingCity::crossingPedestrian) {
                check((i<6&&a.position.z<=-15.5f&&a.position.z>=-16.4f)||
                      (i>=6&&a.position.z<=-28.8f&&a.position.z>=-29.7f),"ordinary walkers remain on the two sidewalks");
            } else if(a.position.z>-27.4f&&a.position.z<-16.8f) {
                check(std::abs(a.position.x-EndingCity::crossingX)<.001f,"road crossing stays within the zebra stripes");
                check(EndingCity::crossingActive(time),"crossing person is protected by a traffic stop");
            }
            const float dx=b.position.x-a.position.x,dz=b.position.z-a.position.z;
            const float step=std::hypot(dx,dz);
            if(step<1) {
                check(std::abs(step/dt-a.speed)<.002f,"walking translates at physical speed through turns");
                check((dx*(-std::sin(a.yaw))+dz*(-std::cos(a.yaw)))/step>.998f,"people face their actual walking direction");
                check(std::abs(std::remainder(b.yaw-a.yaw,2*PI))<.031f,"pedestrian turns have no orientation snap");
            }
            check(std::hypot(a.position.x-camera.eye.x,a.position.z-camera.eye.z)>1.25f,"camera does not collide with pedestrians");
            check(a.gait>=0&&a.gait<=2*PI&&std::abs(a.headYaw)<=.901f,"gait and head turn remain bounded");
            for(int j=i+1;j<EndingCity::pedestrianCount;++j) {
                const auto other=EndingCity::pedestrian(j,time);
                check(std::hypot(cyclicDistance(a.position.x,other.position.x),a.position.z-other.position.z)>.72f,"pedestrians leave shoulder and bag clearance");
            }
            for(int j=0;j<EndingCity::carCount;++j) {
                const auto vehicle=EndingCity::car(j,time);
                // Maximum car envelope includes mirrors; pedestrians include arms/bag.
                const float xx=std::max(0.f,cyclicDistance(a.position.x,vehicle.position.x)-2.16f);
                const float zz=std::max(0.f,std::abs(a.position.z-vehicle.position.z)-1.01f);
                check(std::hypot(xx,zz)>.40f,"vehicles cannot touch a crossing pedestrian");
            }
            check(same(a,EndingCity::pedestrian(i,time)),"same paused time freezes all walking parts exactly");
        }
        for(int i=0;i<EndingCity::carCount;++i) {
            const auto a=EndingCity::car(i,time),b=EndingCity::car(i,time+dt);
            check(finite(a)&&a.speed>=0&&a.speed<=7,"traffic speed and pose are finite");
            check(a.position.y==-.15f&&a.position.x>=-90&&a.position.x<=90,"cars rest on the road and remain bounded");
            check(a.position.z==(i<3?-20.f:-24.f),"traffic stays in its assigned lane");
            const float dx=b.position.x-a.position.x;
            if(std::abs(dx)<1) {
                check(std::abs(std::abs(dx)/dt-a.speed)<.047f,"cars move at their rolling speed, including smooth acceleration");
                check(dx*(-std::sin(a.yaw))>=0,"cars face their driving direction");
                const float wheelStep=std::remainder(b.gait-a.gait,2*PI);
                check(std::abs(std::abs(dx)-wheelStep*.32f)<.00002f,"wheel travel matches road travel while slowing and stopping");
            }
            check(std::abs(b.speed-a.speed)<.091f,"braking and acceleration have bounded speed changes");
            for(int j=i+1;j<EndingCity::carCount;++j) {
                const auto other=EndingCity::car(j,time);
                if(a.position.z==other.position.z)
                    check(cyclicDistance(a.position.x,other.position.x)>30,"same-lane cars cannot overlap, including wrap boundary");
            }
            check(same(a,EndingCity::car(i,time)),"same paused time freezes traffic exactly");
        }
    }
    for(float time:{-1.f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),1e30f})
        for(int i=0;i<12;++i)check(finite(EndingCity::pedestrian(i,time))&&finite(EndingCity::car(i,time)),"invalid and huge samples stay finite");
    check(same(EndingCity::car(0,-1),EndingCity::car(0,0)),"negative scene time clamps to start");
    check(same(EndingCity::pedestrian(0,-1),EndingCity::pedestrian(0,0)),"negative pedestrian time clamps to start");
    check(EndingCity::groundHeight(17.55f,-24)==-.15f&&EndingCity::groundHeight(17.55f,-28)==0,"foot IK distinguishes asphalt from pavement");
    check(EndingCity::groundHeight(17.55f,-27)==.035f&&EndingCity::groundHeight(17.55f,-17)==.035f,"foot IK includes the raised curb lip");
    // The story beat reads during the late street pan: walk, yield, look, continue.
    check(EndingCity::trafficAmber(30)&&!EndingCity::crossingActive(30),"amber precedes the crossing");
    check(EndingCity::crossingActive(35)&&!EndingCity::trafficAmber(35),"crossing has an unambiguous stop signal");
    check(!EndingCity::crossingActive(40)&&!EndingCity::trafficAmber(40),"traffic signal releases after the road is clear");
    check(EndingCity::pedestrian(11,31).position.z<-27&&EndingCity::pedestrian(11,39).position.z>-17,"pedestrian traverses the complete road before fade");
    check(EndingCity::car(2,30).speed>0&&EndingCity::car(2,30).speed<6,"visible car really decelerates");
    check(same(EndingCity::car(2,32),EndingCity::car(2,38)),"stopped car position, wheel phase and brake lamps hold still");
    check(EndingCity::car(2,40).speed>0&&EndingCity::car(2,40).speed<6,"car rolls away smoothly after yielding");
    check(std::abs(EndingCity::pedestrian(8,34).headYaw)>.5f&&EndingCity::pedestrian(8,38).headYaw==0,"bystander glances toward camera then looks forward");
    // There must be street activity in the doorway view and later city shots.
    for(float time:{26.f,32.f,37.f}) {
        int walkers=0,cars=0;
        for(int i=0;i<12;++i)if(std::abs(EndingCity::pedestrian(i,time).position.x)<20)++walkers;
        for(int i=0;i<6;++i)if(std::abs(EndingCity::car(i,time).position.x)<20)++cars;
        check(walkers>=4&&cars>=1,"central street has people and cars at each city milestone");
    }
}

static bool sameFoot(const CityFootPose& a,const CityFootPose& b) {
    return a.position.x==b.position.x&&a.position.y==b.position.y&&a.position.z==b.position.z&&
           a.yaw==b.yaw&&a.phase==b.phase&&a.supportHeight==b.supportHeight&&a.planted==b.planted;
}
static void feet() {
    constexpr float dt=.001f;
    float maxFootStep=0,maxPelvisStep=0,maxStanceDrift=0,maxLegLength=0,maxLegAt=0;int maxLegActor=0;
    for(int actor=0;actor<EndingCity::pedestrianCount;++actor) {
        auto previous=EndingCityFeet::sample(actor,0);
        for(int frame=1;frame<=42000;++frame) {
            const float time=frame*dt;
            const auto body=EndingCity::pedestrian(actor,time);
            const auto pose=EndingCityFeet::sample(actor,time),again=EndingCityFeet::sample(actor,time);
            check(std::isfinite(pose.pelvisHeight)&&pose.pelvisHeight==again.pelvisHeight,"pelvis support is finite and deterministic");
            const float bodyChange=std::abs(pose.pelvisHeight-previous.pelvisHeight);
            maxPelvisStep=std::max(maxPelvisStep,bodyChange);
            check(bodyChange<.008f,"curb support cannot pop the pelvis between frames");
            for(int side=0;side<2;++side) {
                const auto& foot=pose.feet[side];const auto& old=previous.feet[side];
                check(std::isfinite(foot.position.x)&&std::isfinite(foot.position.y)&&std::isfinite(foot.position.z)&&std::isfinite(foot.yaw),"world foot pose stays finite");
                check(sameFoot(foot,again.feet[side]),"pause/seek produces exactly the same planted and swinging feet");
                const float dx=cyclicDistance(foot.position.x,old.position.x),dy=foot.position.y-old.position.y,dz=foot.position.z-old.position.z;
                const float step=std::sqrt(dx*dx+dy*dy+dz*dz);
                maxFootStep=std::max(maxFootStep,step);
                check(step<.014f,"curb foot swing is continuous without ankle teleports");
                if(foot.planted&&old.planted&&foot.phase>=old.phase) {
                    maxStanceDrift=std::max(maxStanceDrift,step);
                    check(step<.00003f,"stance footprint remains anchored in the world, including turns");
                }
                const float radial=std::hypot(cyclicDistance(foot.position.x,body.position.x),foot.position.z-body.position.z);
                check(radial<.68f,"world contacts remain near their actor even at street wrapping");
                const float height=EndingCityFeet::height(actor),width=EndingCityFeet::width(actor);
                const float rx=foot.position.x-body.position.x,rz=foot.position.z-body.position.z;
                const Vec3 ankle{(rx*std::cos(body.yaw)-rz*std::sin(body.yaw))/width,
                                 (foot.position.y-body.position.y)/height,rx*std::sin(body.yaw)+rz*std::cos(body.yaw)};
                const float bounce=.009f*(1-std::cos(body.gait*2))+(pose.pelvisHeight-body.position.y)/height;
                const Vec3 hip{(side?1:-1)*.105f,.81f+bounce,0},leg=ankle-hip;
                const float legLength=std::max(.40f,std::sqrt(dot(leg,leg))*.501f);
                if(legLength>maxLegLength){maxLegLength=legLength;maxLegAt=time;maxLegActor=actor;}
                check(legLength<.45f,"curb and turning IK stays within 12.5 percent of the nominal leg length");
                const float floor=EndingCityFeet::soleGround(foot.position,foot.yaw,EndingCityFeet::width(actor));
                const float sole=foot.position.y-.056f*EndingCityFeet::height(actor);
                check(sole>=floor-.003f,"shoe clears asphalt, pavement and the full raised curb lip");
                if(foot.planted)check(std::abs(sole-floor)<.003f,"planted shoe rests at its physical support height");
            }
            previous=pose;
        }
    }
    std::cout<<"City foot checks: 504000 actor samples; max ankle movement/1ms="<<maxFootStep
             <<"m, max pelvis change/1ms="<<maxPelvisStep<<"m, max planted drift="<<maxStanceDrift<<"m; max leg segment="<<maxLegLength<<"m at "<<maxLegAt<<"s (actor "<<maxLegActor<<").\n";
}

int main(){try{
    timeline();city();feet();
    std::cout<<"Ending tests passed: 42s timeline, pause/replay, door/camera clearance, continuous city departure, rounded crossing, vehicle yielding/clearance, synchronized wheels, head glance and deterministic bounded animation.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
