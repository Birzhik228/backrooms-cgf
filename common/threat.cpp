#include "threat.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <queue>
#include <utility>

namespace br {
namespace {
constexpr double PI=3.14159265358979323846;
constexpr double GRID=.75;
constexpr float MEMORY_SECONDS=5.0f;
constexpr float SIGHT_RANGE=22.0f;
constexpr float WANDER_SPEED=1.1f;
constexpr float CHASE_SPEED=3.3f;
constexpr float SPAWN_GRACE=8.0f;
constexpr double CATCH_DISTANCE=.62;

bool segmentClear(const World& world,double ax,double az,double bx,double bz,double radius) {
    const double distance=std::hypot(bx-ax,bz-az);
    if (!std::isfinite(distance) || distance>240) return false;
    const int count=std::max(1,static_cast<int>(std::ceil(distance/(radius<.1?.10:.20))));
    for (int i=0;i<=count;++i) {
        const double t=static_cast<double>(i)/count;
        if (world.blocked(ax+(bx-ax)*t,az+(bz-az)*t,radius)) return false;
    }
    return true;
}

using Node=std::pair<std::int64_t,std::int64_t>;
Node nodeAt(double x,double z) {
    return {static_cast<std::int64_t>(std::floor(x/GRID)),static_cast<std::int64_t>(std::floor(z/GRID))};
}
WorldPoint pointAt(Node node) { return {(node.first+.5)*GRID,(node.second+.5)*GRID}; }
double distanceSquared(double ax,double az,double bx,double bz) {
    return (bx-ax)*(bx-ax)+(bz-az)*(bz-az);
}
}

const char* threatStateName(ThreatState state) {
    switch (state) {
    case ThreatState::Dormant:return "DORMANT";
    case ThreatState::Wander:return "WANDERING";
    case ThreatState::Chase:return "CHASING";
    case ThreatState::Search:return "SEARCHING";
    case ThreatState::Caught:return "CAUGHT";
    }
    return "UNKNOWN";
}

void Threat::reset(std::uint64_t seed) {
    seed_=seed; random_=seed^0xd1b54a32d192ed03ULL;
    x_=z_=goalX_=goalZ_=lastX_=lastZ_=0;
    yaw_=speed_=age_=repath_=wanderWait_=spawnRetry_=0;
    sinceSense_=100; state_=ThreatState::Dormant;
    seesPlayer_=hasGoal_=false; path_.clear(); pathIndex_=pathNodesVisited_=0;
}

void Threat::resetAt(double x,double z,float yaw) {
    reset(seed_);
    if (!std::isfinite(x) || !std::isfinite(z) || !std::isfinite(yaw)) return;
    x_=x; z_=z; yaw_=yaw; age_=SPAWN_GRACE;
    state_=ThreatState::Wander;
}

double Threat::random01() {
    random_^=random_>>12; random_^=random_<<25; random_^=random_>>27;
    return static_cast<double>((random_*2685821657736338717ULL)>>11)*(1.0/9007199254740992.0);
}

bool Threat::visibleSegment(const World& world,double ax,double az,double bx,double bz) {
    return segmentClear(world,ax,az,bx,bz,.035);
}

bool Threat::playerCanSee(const World& world,const ThreatPlayer& player,double x,double z) const {
    const double dx=x-player.x,dz=z-player.z,distance=std::hypot(dx,dz);
    if (distance<2) return true;
    const double length=std::hypot(player.forwardX,player.forwardZ);
    // Wider than the playable horizontal FOV, allowing for the model's width.
    if (length>.01 && (dx*player.forwardX+dz*player.forwardZ)/(distance*length)<.25) return false;
    return visibleSegment(world,player.x,player.z,x,z);
}

bool Threat::spawn(const World& world,const ThreatPlayer& player) {
    const double oldX=x_,oldZ=z_;
    for (int attempt=0;attempt<8;++attempt) {
        const double angle=random01()*2*PI;
        const double distance=SPAWN_MIN_DISTANCE+random01()*(SPAWN_MAX_DISTANCE-SPAWN_MIN_DISTANCE);
        const double candidateX=player.x+std::sin(angle)*distance;
        const double candidateZ=player.z-std::cos(angle)*distance;
        if (world.blocked(candidateX,candidateZ,RADIUS) || playerCanSee(world,player,candidateX,candidateZ)) continue;
        x_=candidateX; z_=candidateZ;
        // An unobstructed coordinate alone is insufficient: prove a route back
        // to the player's connected space before exposing this spawn.
        if (!plan(world,player.x,player.z)) { x_=oldX; z_=oldZ; continue; }
        yaw_=static_cast<float>(std::atan2(player.x-x_,-(player.z-z_)));
        state_=ThreatState::Wander; sinceSense_=100; speed_=0; seesPlayer_=false;
        // Follow part of this valid route during the first patrol, so a spawn
        // in a distant room eventually becomes an encounter.
        goalX_=player.x; goalZ_=player.z; hasGoal_=true; repath_=1;
        return true;
    }
    x_=oldX; z_=oldZ; path_.clear(); pathIndex_=0;
    return false;
}

bool Threat::plan(const World& world,double goalX,double goalZ) {
    path_.clear(); pathIndex_=0; pathNodesVisited_=0;
    if (world.blocked(x_,z_,RADIUS) || world.blocked(goalX,goalZ,RADIUS)) return false;
    if (segmentClear(world,x_,z_,goalX,goalZ,RADIUS)) {
        path_.push_back({goalX,goalZ});
        return true;
    }

    // A* uses a fixed metric grid, and World::blocked rejects every unloaded
    // chunk. Both expanded nodes and retained frontier entries have hard caps.
    struct Record { double cost; Node parent; bool closed; };
    struct Candidate { double score; Node node; };
    struct Later { bool operator()(const Candidate& a,const Candidate& b) const {
        return a.score>b.score || (a.score==b.score && a.node>b.node);
    } };
    std::map<Node,Record> records;
    std::map<Node,bool> walkable;
    std::priority_queue<Candidate,std::vector<Candidate>,Later> frontier;
    const Node rawStart=nodeAt(x_,z_),rawGoal=nodeAt(goalX,goalZ);
    Node start{},goal{};
    const auto attach=[&](Node raw,double x,double z,Node& result) {
        double closest=std::numeric_limits<double>::infinity(); bool found=false;
        for (int dz=-1;dz<=1;++dz) for (int dx=-1;dx<=1;++dx) {
            const Node n{raw.first+dx,raw.second+dz}; const auto p=pointAt(n);
            const double distance=distanceSquared(x,z,p.x,p.z);
            if (distance<closest && segmentClear(world,x,z,p.x,p.z,RADIUS)) {
                result=n; closest=distance; found=true;
            }
        }
        return found;
    };
    if (!attach(rawStart,x_,z_,start) || !attach(rawGoal,goalX,goalZ,goal)) return false;
    const auto heuristic=[&](Node n) {return GRID*(std::abs(n.first-goal.first)+std::abs(n.second-goal.second));};
    records.emplace(start,Record{0,start,false}); frontier.push({heuristic(start),start});
    static constexpr std::array<Node,4> neighbours{{{1,0},{0,1},{-1,0},{0,-1}}};
    bool reached=false;
    while (!frontier.empty() && pathNodesVisited_<MAX_PATH_NODES) {
        const auto current=frontier.top().node; frontier.pop();
        auto& record=records.at(current); if (record.closed) continue;
        record.closed=true; ++pathNodesVisited_;
        if (current==goal) { reached=true; break; }
        const auto p=pointAt(current);
        for (const auto& offset:neighbours) {
            const Node next{current.first+offset.first,current.second+offset.second};
            const auto previous=records.find(next);
            if (previous!=records.end() && previous->second.closed) continue;
            const auto q=pointAt(next);
            auto free=walkable.find(next);
            if (free==walkable.end()) {
                if (walkable.size()>=MAX_PATH_NODES*3) continue;
                free=walkable.emplace(next,!world.blocked(q.x,q.z,RADIUS)).first;
            }
            if (!free->second || !segmentClear(world,p.x,p.z,q.x,q.z,RADIUS)) continue;
            const double cost=record.cost+GRID;
            if (previous!=records.end() && cost>=previous->second.cost) continue;
            if (records.size()>=MAX_PATH_NODES*2 || frontier.size()>=MAX_PATH_NODES*2) continue;
            records[next]={cost,current,false}; frontier.push({cost+heuristic(next),next});
        }
    }
    if (!reached) return false;
    std::vector<WorldPoint> reverse;
    for (Node n=goal;;n=records.at(n).parent) {
        reverse.push_back(pointAt(n)); if (n==start) break;
    }
    path_.assign(reverse.rbegin(),reverse.rend()); path_.push_back({goalX,goalZ});
    // Remove short collinear runs without a costly long-distance visibility
    // pass. Movement still performs a fresh swept collision test every frame.
    std::vector<WorldPoint> compact;
    for (const auto& point:path_) {
        if (compact.size()>=2) {
            const auto& a=compact[compact.size()-2]; const auto& b=compact.back();
            const double cross=(b.x-a.x)*(point.z-b.z)-(b.z-a.z)*(point.x-b.x);
            const double forward=(b.x-a.x)*(point.x-b.x)+(b.z-a.z)*(point.z-b.z);
            if (std::abs(cross)<1e-8 && forward>=0) compact.pop_back();
        }
        compact.push_back(point);
    }
    path_=std::move(compact);
    return true;
}

void Threat::chooseWanderGoal(const World& world,const ThreatPlayer& player) {
    hasGoal_=false;
    for (int attempt=0;attempt<12;++attempt) {
        const double angle=random01()*2*PI;
        const bool distant=std::hypot(x_-player.x,z_-player.z)>20;
        const double distance=6+random01()*9;
        const double gx=(distant?player.x:x_)+std::sin(angle)*distance;
        const double gz=(distant?player.z:z_)-std::cos(angle)*distance;
        if (world.blocked(gx,gz,RADIUS)) continue;
        goalX_=gx; goalZ_=gz; hasGoal_=true; repath_=0;
        return;
    }
    wanderWait_=1;
}

void Threat::walk(const World& world,float dt,float desiredSpeed) {
    const double initialX=x_,initialZ=z_;
    double remaining=desiredSpeed*dt;
    while (remaining>1e-7 && pathIndex_<path_.size()) {
        const auto point=path_[pathIndex_];
        const double dx=point.x-x_,dz=point.z-z_,distance=std::hypot(dx,dz);
        if (distance<.025) { ++pathIndex_; continue; }
        const double step=std::min({remaining,distance,.12});
        const double nextX=x_+dx/distance*step,nextZ=z_+dz/distance*step;
        if (!segmentClear(world,x_,z_,nextX,nextZ,RADIUS)) {
            path_.clear(); pathIndex_=0; repath_=.35f;
            break;
        }
        x_=nextX; z_=nextZ; remaining-=step;
        const float desiredYaw=static_cast<float>(std::atan2(dx,-dz));
        const float delta=std::remainder(desiredYaw-yaw_,static_cast<float>(2*PI));
        yaw_+=std::clamp(delta,-dt*6.f,dt*6.f);
    }
    speed_=dt>0?static_cast<float>(std::hypot(x_-initialX,z_-initialZ))/dt:0;
}

void Threat::update(const World& world,const ThreatPlayer& player,float dt,bool paused) {
    if (paused || caught() || !std::isfinite(dt) || dt<=0 ||
        !std::isfinite(player.x) || !std::isfinite(player.z)) return;
    dt=std::min(dt,.1f);
    age_+=dt; speed_=0; spawnRetry_-=dt; repath_-=dt; wanderWait_-=dt;
    if (!active()) {
        if (age_>=SPAWN_GRACE && spawnRetry_<=0) {
            spawn(world,player); spawnRetry_=2;
        }
        return;
    }
    const double dx=player.x-x_,dz=player.z-z_,distance=std::hypot(dx,dz);
    // Only move the single entity to new streamed space when its old and new
    // positions are both hidden from the player, and never during a chase.
    const bool geometryChanged=distance>25 && world.blocked(x_,z_,RADIUS);
    if ((distance>75 || geometryChanged) && state_==ThreatState::Wander && spawnRetry_<=0 && !playerCanSee(world,player,x_,z_)) {
        spawn(world,player); spawnRetry_=3;
        return;
    }
    const bool clear=distance<=24 && visibleSegment(world,x_,z_,player.x,player.z);
    const double facing=distance>.01?(dx*std::sin(yaw_)-dz*std::cos(yaw_))/distance:1;
    seesPlayer_=clear && distance<=SIGHT_RANGE && (facing>=.57 || distance<1.5);
    const float motion=std::isfinite(player.speed)?std::max(0.f,player.speed):0.f;
    const float noise=std::isfinite(player.noise)?std::clamp(player.noise,0.f,1.f):0.f;
    double hearing=motion>.12?std::clamp(2.0+motion*3.5,0.0,18.0):0;
    if (player.crouched) hearing*=.28;
    hearing=std::max(hearing,20.0*noise);
    // A wall muffles sound but does not make running or a nearby door silent.
    if (!clear) hearing*=.45;
    const bool heard=hearing>0 && distance<hearing;
    if (seesPlayer_ || heard) {
        const bool entering=state_!=ThreatState::Chase;
        state_=ThreatState::Chase; sinceSense_=0;
        if (entering || distanceSquared(lastX_,lastZ_,player.x,player.z)>2.25) repath_=0;
        lastX_=goalX_=player.x; lastZ_=goalZ_=player.z; hasGoal_=true;
    } else {
        sinceSense_+=dt;
        if (state_==ThreatState::Chase) state_=ThreatState::Search;
        if (state_==ThreatState::Search && sinceSense_>=MEMORY_SECONDS) {
            state_=ThreatState::Wander; hasGoal_=false; path_.clear(); pathIndex_=0;
            wanderWait_=1; repath_=0;
        }
    }
    if (distance<CATCH_DISTANCE && clear) {
        state_=ThreatState::Caught; speed_=0; path_.clear(); return;
    }
    if (world.blocked(x_,z_,RADIUS)) {
        // Closing doors must also respect this capsule in the game loop.
        // Never teleport a visible actor to recover from changed geometry.
        path_.clear(); pathIndex_=0; repath_=.35f;
        return;
    }
    if (state_==ThreatState::Wander && (!hasGoal_ || std::hypot(goalX_-x_,goalZ_-z_)<.4) && wanderWait_<=0)
        chooseWanderGoal(world,player);
    if (state_==ThreatState::Search && std::hypot(lastX_-x_,lastZ_-z_)<.45) {
        yaw_+=dt*.8f; hasGoal_=false; path_.clear(); pathIndex_=0;
    }
    if (hasGoal_ && repath_<=0) {
        const bool valid=plan(world,goalX_,goalZ_);
        repath_=state_==ThreatState::Wander?1.8f:.65f;
        if (!valid && state_==ThreatState::Wander) { hasGoal_=false; wanderWait_=1; }
    }
    walk(world,dt,state_==ThreatState::Wander?WANDER_SPEED:CHASE_SPEED);
    // Test after the sweep as well, so high frame times cannot skip contact.
    if (std::hypot(player.x-x_,player.z-z_)<CATCH_DISTANCE && visibleSegment(world,x_,z_,player.x,player.z)) {
        state_=ThreatState::Caught; speed_=0; path_.clear();
    }
}

} // namespace br
