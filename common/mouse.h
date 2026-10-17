#pragma once
#include "../world.h"
#include <algorithm>
#include <cmath>

namespace br {
// One small resident mouse. It keeps its position while nearby chunks stream.
class Mouse {
public:
    double x=0,z=0;
    float yaw=0,animation=0;
    bool active=false,alerted=false;
    // One-frame, quiet discovery cue; only update's return value is an alarm.
    bool ambientCue=false;
    void reset(std::uint64_t seed) { *this=Mouse{}; random_=seed; }
    void releaseFromPresent(double px,double pz) {
        x=px;z=pz;active=true;alerted=true;flee_=1000000.f;ambientClock_=4;
    }
    bool update(const World& world,double px,double pz,float dt,bool suspended,bool hidden,bool allowSpawn=true) {
        ambientCue=false;
        if(suspended || !std::isfinite(dt) || dt<=0) return false;
        dt=std::min(dt,.05f);
        if(!active || std::hypot(px-x,pz-z)>55 || world.blocked(x,z,.12)) {
            active=false;
            if(!allowSpawn)return false;
            const auto cx=std::int64_t(std::floor(px/CELL)),cz=std::int64_t(std::floor(pz/CELL));
            for(const auto offset:{WorldPoint{0,-1},{1,0},{-1,0},{0,1},{0,0}}) {
                const double tx=(cx+offset.x+.5)*CELL,tz=(cz+offset.z+.5)*CELL;
                if(!world.blocked(tx,tz,.18)) { x=tx;z=tz;active=true;break; }
            }
            turn_=0;armed_=true;
        }
        if(!active) return false;
        life_+=dt;
        ambientClock_-=dt;
        cooldown_=std::max(0.f,cooldown_-dt);flee_=std::max(0.f,flee_-dt);
        const double distance=std::hypot(px-x,pz-z);
        if(distance>.85) armed_=true;
        bool contact=!alerted && !hidden && armed_ && cooldown_<=0 && distance<.48;
        for(int i=1;contact && i<8;++i)
            if(world.blocked(x+(px-x)*i/8,z+(pz-z)*i/8,.035)) contact=false;
        if(contact) {
            cooldown_=3;armed_=false;alerted=true;flee_=1000000.f;
            ambientClock_=3.5f;
            yaw=float(std::atan2(x-px,-(z-pz)));turn_=1.4f;
        }
        // Seek the player until the one alarm, then keep escaping.
        const float wanted=float(std::atan2(alerted?x-px:px-x,alerted?pz-z:z-pz));
        const double speed=alerted?2.1:1.65;
        float best=wanted;double bestScore=-1e9;
        for(float offset:{0.f,.45f,-.45f,.9f,-.9f,1.5f,-1.5f,2.2f,-2.2f,3.14159f}) {
            float direction=wanted+offset;double sx=std::sin(direction),sz=-std::cos(direction);
            bool clear=true;
            for(int step=1;step<=6;++step)if(world.blocked(x+sx*step*.10,z+sz*step*.10,.12)){clear=false;break;}
            if(!clear)continue;
            double score=std::cos(offset)*2+.3*std::cos(direction-yaw);
            if(score>bestScore){bestScore=score;best=direction;}
        }
        yaw=best;
        const double dx=std::sin(yaw)*dt*speed,dz=-std::cos(yaw)*dt*speed;
        const double oldX=x,oldZ=z;
        if(!world.blocked(x+dx,z,.12)) x+=dx;
        if(!world.blocked(x,z+dz,.12)) z+=dz;
        if(std::hypot(x-oldX,z-oldZ)<.0001) {yaw+=1.7f;turn_=.5f;}
        const float travelled=float(std::hypot(x-oldX,z-oldZ));
        animation+=travelled*24;
        strideBlend_+=(std::min(1.f,travelled/dt/.35f)-strideBlend_)*(1-std::exp(-dt*12));
        if(ambientClock_<=0 && flee_<=0) {
            ambientCue=true;
            const auto cueHash=hashCell(++cueIndex_,23,random_);
            ambientClock_=3.5f+float(cueHash%2501)*.001f;
        }
        return contact;
    }
    std::vector<Vertex> vertices(double originX,double originZ) const {
        std::vector<Vertex> mesh;
        if(!active) return mesh;
        constexpr float pi=3.141592653589793f;
        auto ellipsoid=[&](float cx,float cy,float cz,float rx,float ry,float rz,float material) {
            auto point=[&](float lat,float lon) {
                const float nx=std::cos(lat)*std::cos(lon),ny=std::sin(lat),nz=std::cos(lat)*std::sin(lon);
                const float len=std::sqrt(nx*nx/(rx*rx)+ny*ny/(ry*ry)+nz*nz/(rz*rz));
                return Vertex{cx+rx*nx,cy+ry*ny,cz+rz*nz,nx/rx/len,ny/ry/len,nz/rz/len,lon,lat+pi/2,material};
            };
            for(int ring=0;ring<6;++ring) for(int side=0;side<10;++side) {
                const float a=-pi/2+ring*pi/6,b=a+pi/6,c=side*2*pi/10,d=c+2*pi/10;
                const Vertex v[]={point(a,c),point(b,c),point(b,d),point(a,d)};
                for(int index:{0,1,2,0,2,3}) mesh.push_back(v[index]);
            }
        };
        const float breath=std::sin(life_*6)*.0025f;
        const float bob=std::abs(std::sin(animation))*strideBlend_*.009f;
        const float sniff=std::pow(std::max(0.f,std::sin(life_*2.1f)),6);
        const float headY=bob+sniff*.013f,headX=std::sin(life_*3.2f)*.007f;
        ellipsoid(0,.16f+bob+breath,0,.125f,.13f+breath,.24f,25);
        ellipsoid(headX,.19f+headY,-.23f,.105f,.10f,.13f,25);
        ellipsoid(headX,.17f+headY+sniff*.004f,-.335f-sniff*.009f,.036f,.028f,.042f,26);
        for(float side:{-1.f,1.f}) {
            const float twitch=std::pow(std::max(0.f,std::sin(life_*2.8f+side)),12)*.015f;
            ellipsoid(headX+side*(.09f+twitch),.285f+headY,-.20f+twitch,.065f,.072f-twitch*.4f,.026f,25);
            ellipsoid(headX+side*(.09f+twitch),.287f+headY,-.222f+twitch,.043f,.049f-twitch*.3f,.008f,26);
            ellipsoid(headX+side*.073f,.218f+headY,-.294f,.016f,.017f,.012f,3);
            for(float end:{-1.f,1.f}) {
                const float step=std::sin(animation+(side*end>0?0:pi))*strideBlend_;
                ellipsoid(side*.087f,.046f+std::max(0.f,step)*.018f,end*.14f+step*.035f,.034f,.043f,.055f,26);
            }
        }
        // A continuous tapered tube avoids gaps on the curved thin tail tip.
        auto tailPoint=[&](float t,float angle) {
            const float phase=t*3+std::sin(life_*4+animation*.3f)*.35f;
            const float slope=(std::sin(phase)+3*t*std::cos(phase))*.15f/.40f;
            const float nx=std::cos(angle)/std::sqrt(1+slope*slope),ny=std::sin(angle),nz=-slope*nx;
            const float radius=.020f*(1-t)+.002f;
            return Vertex{std::sin(phase)*t*.15f+nx*radius,.065f+bob+ny*radius,.12f+t*.40f+nz*radius,
                nx,ny,nz,t,angle,26};
        };
        for(int i=0;i<16;++i) for(int side=0;side<8;++side) {
            const float t=i/16.f,next=(i+1)/16.f,a=side*2*pi/8,b=(side+1)*2*pi/8;
            const Vertex v[]={tailPoint(t,a),tailPoint(t,b),tailPoint(next,b),tailPoint(next,a)};
            for(int index:{0,1,2,0,2,3}) mesh.push_back(v[index]);
        }
        const auto tip=tailPoint(1,0);
        ellipsoid(tip.x,tip.y,tip.z,.004f,.004f,.004f,26);
        const float cs=std::cos(yaw),sn=std::sin(yaw);
        for(auto& v:mesh) {
            const float vx=v.x,vz=v.z,nx=v.nx,nz=v.nz;
            v.x=float(x-originX)+vx*cs-vz*sn;v.z=float(z-originZ)+vx*sn+vz*cs;
            v.nx=nx*cs-nz*sn;v.nz=nx*sn+nz*cs;
        }
        return mesh;
    }
private:
    std::uint64_t random_=42;
    float cooldown_=0,turn_=0,flee_=0;
    float life_=0,strideBlend_=0,ambientClock_=1.8f;
    std::int64_t cueIndex_=0;
    bool armed_=true;
};
} // namespace br
