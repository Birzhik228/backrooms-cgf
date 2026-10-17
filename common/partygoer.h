#pragma once
#include "../world.h"
#include "math.h"
#include "render_limits.h"
#include <array>
#include <queue>
#include <map>

namespace br {
// A small fixed population, with independent gaze/animation and bounded routing.
class Partygoers {
public:
    static constexpr int COUNT=4;
    static constexpr float SPEED=1.95f;
    static constexpr double RADIUS=.34;
    struct Actor {
        double x=0,z=0;
        float yaw=0,phase=0,repath=0;
        bool active=false,observed=false;
        std::vector<WorldPoint> path;
        size_t next=0;
    };
    std::array<Actor,COUNT> actors{};
    double travelled=0;
    int frozenFrames=0;
    void reset() { *this=Partygoers{}; }
    // Height-aware wall rays: low furniture cannot hide a visible head.
    static bool clearSight(const World& world,double ax,double ay,double az,double bx,double by,double bz) {
        for(const auto& [coord,c]:world.chunks()) {
            const double ox=coord.x*CHUNK_SIZE,oz=coord.z*CHUNK_SIZE;
            if(std::max(ax,bx)<ox-.2 || std::min(ax,bx)>ox+CHUNK_SIZE+.2 ||
               std::max(az,bz)<oz-.2 || std::min(az,bz)>oz+CHUNK_SIZE+.2)continue;
            for(const auto& wall:c.sightBlockers) {
                double enter=0,leave=1;
                auto slab=[&](double start,double end,double lo,double hi) {
                    double d=end-start;
                    if(std::abs(d)<1e-9)return start>=lo && start<=hi;
                    double a=(lo-start)/d,b=(hi-start)/d;if(a>b)std::swap(a,b);
                    enter=std::max(enter,a);leave=std::min(leave,b);return enter<=leave;
                };
                if(slab(ax,bx,ox+wall.footprint.minX,ox+wall.footprint.maxX) &&
                   slab(az,bz,oz+wall.footprint.minZ,oz+wall.footprint.maxZ) &&
                   slab(ay,by,wall.bottom,wall.top) && enter<.999 && leave>.001)return false;
            }
        }
        return true;
    }
    static bool seen(const World& world,const Actor& a,double px,double pz,float eyeY,Vec3 forward,float fov,float aspect) {
        Vec3 right=normalize(cross(forward,{0,1,0})),up=normalize(cross(right,forward));
        const float tanY=std::tan(fov*PI/360),tanX=tanY*aspect;
        // Sample the silhouette, including the hem when looking down; a visible
        // part freezes the whole figure. The margin prevents edge flicker.
        for(const Vec3 sample:{Vec3{0,2.05f,0},{-.32f,1.65f,0},{.32f,1.65f,0},{0,1.35f,0},
                              {-.55f,1.1f,0},{.55f,1.1f,0},{0,.35f,0},{-.35f,.65f,0},{.35f,.65f,0}}) {
            // Rotate sample positions with the body and reject protruding
            // samples if the actual body at this height is behind a wall.
            const float sx=sample.x*std::cos(a.yaw)-sample.z*std::sin(a.yaw);
            const float sz=sample.x*std::sin(a.yaw)+sample.z*std::cos(a.yaw);
            if(!clearSight(world,px,eyeY,pz,a.x,sample.y,a.z))continue;
            Vec3 delta{float(a.x-px)+sx,sample.y-eyeY,float(a.z-pz)+sz};
            if(dot(delta,delta)>=SCENE_FOG_END*SCENE_FOG_END)continue;
            float depth=dot(delta,forward);
            if(depth<=0 || depth>110)continue;
            if(std::abs(dot(delta,right))>depth*tanX+.12f || std::abs(dot(delta,up))>depth*tanY+.12f)continue;
            if(clearSight(world,px,eyeY,pz,a.x+sx,sample.y,a.z+sz))return true;
        }
        return false;
    }
    static bool walkable(const World& world,double ax,double az,double bx,double bz) {
        int steps=std::max(1,int(std::ceil(std::hypot(bx-ax,bz-az)/.15)));
        for(int i=0;i<=steps;++i) {
            double t=double(i)/steps,x=ax+(bx-ax)*t,z=az+(bz-az)*t;
            if(world.blocked(x,z,RADIUS) || world.headClearance(x,z,RADIUS)<2.35)return false;
        }
        return true;
    }
    void update(const World& world,double px,double pz,float eyeY,Vec3 forward,float fov,float aspect,float dt,bool paused) {
        if(!world.isFun()) {reset();return;}
        if(paused || !std::isfinite(dt) || dt<=0)return;
        dt=std::min(dt,.05f);
        for(int index=0;index<COUNT;++index) {
            auto& a=actors[index];
            if(!a.active || (std::hypot(a.x-px,a.z-pz)>65 && !seen(world,a,px,pz,eyeY,forward,fov,aspect))) {
                a={};
                const auto cx=std::int64_t(std::floor(px/CELL)),cz=std::int64_t(std::floor(pz/CELL));
                const auto hash=hashCell(cx,cz,world.seed()+index*7919);
                for(int k=0;k<80;++k) {
                    int n=int((hash+k*17)%80),dx=n%9-4,dz=n/9-4;
                    Actor candidate;candidate.x=(cx+dx+.5)*CELL;candidate.z=(cz+dz+.5)*CELL;
                    const double distance=std::hypot(candidate.x-px,candidate.z-pz);
                    if(distance<10 || distance>32 || world.blocked(candidate.x,candidate.z,RADIUS))continue;
                    bool crowded=false;for(const auto& other:actors)if(other.active && std::hypot(other.x-candidate.x,other.z-candidate.z)<3)crowded=true;
                    if(crowded || seen(world,candidate,px,pz,eyeY,forward,fov,aspect))continue;
                    a=candidate;a.active=true;a.yaw=float(std::atan2(px-a.x,-(pz-a.z)));a.repath=index*.2f;break;
                }
            }
            if(!a.active)continue;
            a.observed=seen(world,a,px,pz,eyeY,forward,fov,aspect);
            if(a.observed){++frozenFrames;continue;}
            // Contact is deliberately non-lethal in this version: no capture,
            // camera takeover, audio stinger, or jumpscare state is triggered.
            if(std::hypot(a.x-px,a.z-pz)<.95)continue;
            a.repath-=dt;
            if(a.repath<=0){route(world,a,px,pz);a.repath=.9f+index*.07f;}
            if(a.next>=a.path.size())continue;
            auto target=a.path[a.next];double dx=target.x-a.x,dz=target.z-a.z,d=std::hypot(dx,dz);
            if(d<.08){++a.next;continue;}
            float wanted=float(std::atan2(dx,-dz));
            a.yaw+=std::clamp(std::remainder(wanted-a.yaw,2*PI),-dt*2.8f,dt*2.8f);
            if(std::abs(std::remainder(wanted-a.yaw,2*PI))>.15f)continue;
            double step=std::min(double(SPEED*dt),d),nx=a.x+dx/d*step,nz=a.z+dz/d*step;
            bool crowded=false;for(const auto& other:actors)if(&other!=&a && other.active && std::hypot(other.x-nx,other.z-nz)<.72)crowded=true;
            if(crowded || !walkable(world,a.x,a.z,nx,nz))continue;
            a.x=nx;a.z=nz;a.phase+=float(step)*5;travelled+=step;
        }
    }
    int activeCount()const {int n=0;for(const auto& a:actors)n+=a.active;return n;}
private:
    using Cell=std::pair<std::int64_t,std::int64_t>;
    static WorldPoint center(Cell c){return {(c.first+.5)*CELL,(c.second+.5)*CELL};}
    static void route(const World& world,Actor& a,double px,double pz) {
        a.path.clear();a.next=0;
        if(walkable(world,a.x,a.z,px,pz)){a.path.push_back({px,pz});return;}
        Cell start{std::int64_t(std::floor(a.x/CELL)),std::int64_t(std::floor(a.z/CELL))};
        Cell goal{std::int64_t(std::floor(px/CELL)),std::int64_t(std::floor(pz/CELL))};
        std::queue<Cell> queue;std::map<Cell,Cell> parent;queue.push(start);parent[start]=start;
        while(!queue.empty() && parent.size()<256 && !parent.count(goal)) {
            auto c=queue.front();queue.pop();auto p=center(c);
            for(auto delta:{Cell{1,0},{-1,0},{0,1},{0,-1}}) {
                Cell n{c.first+delta.first,c.second+delta.second};if(parent.count(n))continue;
                auto q=center(n);
                // Fun generation guarantees clear centre-to-door crosses.
                // Test endpoints and the shared opening, avoiding expensive
                // full sweeps on every edge of the bounded route search.
                if(world.blocked(q.x,q.z,RADIUS) || world.blocked((p.x+q.x)*.5,(p.z+q.z)*.5,RADIUS))continue;
                parent[n]=c;queue.push(n);
            }
        }
        if(!parent.count(goal))return;
        std::vector<WorldPoint> reverse;
        for(Cell c=goal;c!=start;c=parent.at(c))reverse.push_back(center(c));
        auto first=center(start);
        if(!walkable(world,a.x,a.z,first.x,first.z))return;
        a.path.push_back(first);
        for(auto i=reverse.rbegin();i!=reverse.rend();++i)a.path.push_back(*i);
        auto last=center(goal);if(walkable(world,last.x,last.z,px,pz))a.path.push_back({px,pz});
    }
};
} // namespace br
