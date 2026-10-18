#pragma once
#include "partygoer.h"

namespace br {
// The garage's darkness has no body: only three pairs of floating eyes.
// Flashlight tests use the physical beam cone, distance and occlusion, not gaze.
class Darkness {
public:
    static constexpr float CHASE_SPEED=4.8f; // Player sprint is 4.0 m/s.
    struct Eyes { double x=0,z=0;float y=1.7f,phase=0,cooldown=0,emerging=0;bool active=false; };
    std::array<Eyes,3> eyes{};
    int dispelled=0,spawned=0;
    void reset() { *this=Darkness{}; }
    int activeCount() const { int n=0;for(const auto& a:eyes)n+=a.active;return n; }
    static bool safe(const World& w,double x,double z) {
        if(!w.isParking())return true;
        if(w.breakerPowered(parkingSector(int64_t(std::floor(x/CELL)),int64_t(std::floor(z/CELL)))))return true;
        // Clearly lit refuges at the entrance and each electrical cabinet.
        if(std::hypot(x-CELL*.5,z-CELL*.5)<2.6)return true;
        for(int i=0;i<3;++i) {const auto p=w.breakerLocation(i);if(std::hypot(x-p.x,z-p.z)<2.1)return true;}
        return false;
    }
    static bool inBeam(const World& w,const Eyes& a,double x,float y,double z,Vec3 forward,bool flashlight) {
        if(!flashlight)return false;
        const Vec3 delta{float(a.x-x),a.y-y,float(a.z-z)};
        const float distance=std::sqrt(dot(delta,delta));
        if(distance>.01f && distance<=18 && dot(delta*(1.f/distance),normalize(forward))>=.955f)
            return Partygoers::clearSight(w,x,y,z,a.x,a.y,a.z);
        return false;
    }
    bool update(const World& w,double x,double z,float eyeY,Vec3 forward,bool flashlight,float dt,bool paused) {
        if(!w.isParking()){reset();return false;}
        if(paused || !std::isfinite(dt) || dt<=0)return false;
        dt=std::min(dt,.05f);age_+=dt;
        for(int index=0;index<int(eyes.size());++index) {
            auto& a=eyes[index];a.cooldown=std::max(0.f,a.cooldown-dt);
            if(!a.active && age_>2+index*1.5f && a.cooldown<=0) {
                a.cooldown=.5f;
                for(int trial=0;trial<32;++trial) {
                    const auto h=hashCell(++roll_,index,w.seed()^0x4441524bULL);
                    // Try the visible edges of the view first, outside the beam.
                    // The player sees the arrival before the pursuit starts.
                    const double angle=trial<24?std::atan2(forward.z,forward.x)+(h&1?1:-1)*(.45+double(h%351)*.001):double(h%6284)*.001;
                    const double radius=12+double((h>>16)%5);
                    const double ax=x+std::cos(angle)*radius,az=z+std::sin(angle)*radius;
                    if(safe(w,ax,az) || w.blocked(ax,az,.2) || !Partygoers::clearSight(w,x,eyeY,z,ax,1.7,az))continue;
                    a.x=ax;a.z=az;a.phase=float((h>>24)%628)*.01f;a.y=1.7f;
                    a.emerging=.65f;a.active=true;++spawned;break;
                }
            }
            if(!a.active)continue;
            a.phase+=dt*1.4f;a.y=1.7f+.13f*std::sin(a.phase);
            if(safe(w,a.x,a.z)) {a.active=false;a.cooldown=6;continue;}
            if(inBeam(w,a,x,eyeY,z,forward,flashlight)) {
                a.active=false;a.cooldown=4+index;++dispelled;continue;
            }
            if(a.emerging>0){a.emerging=std::max(0.f,a.emerging-dt);continue;}
            const double distance=std::hypot(x-a.x,z-a.z);
            if(distance>35) {a.active=false;a.cooldown=3;continue;}
            // A lit refuge stops the approach; the eyes wait outside its edge.
            if(!safe(w,x,z) && distance<.65 && Partygoers::clearSight(w,x,eyeY,z,a.x,a.y,a.z))return true;
            const double angle=std::atan2(z-a.z,x-a.x),step=dt*CHASE_SPEED;
            double best=-1e9,bx=a.x,bz=a.z;
            for(float offset:{0.f,.45f,-.45f,.9f,-.9f,1.5f,-1.5f,2.1f,-2.1f}) {
                const double nx=a.x+std::cos(angle+offset)*step,nz=a.z+std::sin(angle+offset)*step;
                if(safe(w,nx,nz) || w.blocked(nx,nz,.18))continue;
                const double score=std::cos(offset);
                if(score>best){best=score;bx=nx;bz=nz;}
            }
            a.x=bx;a.z=bz;
        }
        return false;
    }
    std::vector<Vertex> vertices(double originX,double originZ,double px,double pz) const {
        std::vector<Vertex> out;out.reserve(36);
        for(const auto& a:eyes)if(a.active) {
            const Vec3 face=normalize(Vec3{float(px-a.x),0,float(pz-a.z)});
            const Vec3 right{face.z,0,-face.x};
            for(float side:{-.19f,.19f}) {
                const Vec3 center{float(a.x-originX)+right.x*side,a.y,float(a.z-originZ)+right.z*side};
                const Vec3 q[]={center-right*.13f-Vec3{0,.065f,0},center+right*.13f-Vec3{0,.065f,0},
                    center+right*.13f+Vec3{0,.065f,0},center-right*.13f+Vec3{0,.065f,0}};
                const float uv[][2]={{0,0},{1,0},{1,1},{0,1}};
                for(int i:{0,1,2,0,2,3})out.push_back({q[i].x,q[i].y,q[i].z,face.x,0,face.z,uv[i][0],uv[i][1],91});
            }
        }
        return out;
    }
    static std::vector<Vertex> scareVertices(Vec3 eye,Vec3 forward,float lunge) {
        const Vec3 right=normalize(cross(forward,{0,1,0})),up=normalize(cross(right,forward));
        const Vec3 center=eye+forward*.9f;
        const float size=.30f+.83f*lunge;
        const Vec3 q[]={center-right*size-up*size,center+right*size-up*size,
                        center+right*size+up*size,center-right*size+up*size};
        const float uv[][2]={{0,0},{1,0},{1,1},{0,1}};
        std::vector<Vertex> out;
        for(int i:{0,1,2,0,2,3})out.push_back({q[i].x,q[i].y,q[i].z,-forward.x,-forward.y,-forward.z,uv[i][0],uv[i][1],92});
        return out;
    }
private:
    float age_=0;
    std::int64_t roll_=0;
};
} // namespace br
