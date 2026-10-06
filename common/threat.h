#pragma once

#include "../world.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace br {

enum class ThreatState { Dormant, Wander, Chase, Search, Caught };
const char* threatStateName(ThreatState state);

// Noise is a one-frame event in [0,1] (for example, opening a door). Speed is
// actual displacement per second, so pushing against a wall is not audible.
struct ThreatPlayer {
    double x=0,z=0;
    float forwardX=0,forwardZ=-1,speed=0;
    bool crouched=false;
    float noise=0;
};

class Threat {
public:
    static constexpr double RADIUS=.26;
    static constexpr double SPAWN_MIN_DISTANCE=16.0;
    static constexpr double SPAWN_MAX_DISTANCE=22.0;
    static constexpr std::size_t MAX_PATH_NODES=4096;
    explicit Threat(std::uint64_t seed=42) { reset(seed); }
    void reset(std::uint64_t seed);
    // Deterministic fixture/capture entry point; ordinary games use reset().
    void resetAt(double x,double z,float yaw=0);
    void update(const World& world,const ThreatPlayer& player,float dt,bool paused=false);

    bool active() const { return state_!=ThreatState::Dormant; }
    double x() const { return x_; }
    double z() const { return z_; }
    // Radians: model forward is (sin(yaw), -cos(yaw)).
    float yaw() const { return yaw_; }
    float speed() const { return speed_; }
    ThreatState state() const { return state_; }
    bool caught() const { return state_==ThreatState::Caught; }
    bool canSeePlayer() const { return seesPlayer_; }
    float timeSinceSense() const { return sinceSense_; }
    std::size_t pathNodesVisited() const { return pathNodesVisited_; }
    std::size_t pathPointCount() const { return path_.size(); }
    static bool visibleSegment(const World& world,double ax,double az,double bx,double bz);

private:
    std::uint64_t seed_=42,random_=42;
    double x_=0,z_=0,goalX_=0,goalZ_=0,lastX_=0,lastZ_=0;
    float yaw_=0,speed_=0,age_=0,sinceSense_=100,repath_=0,wanderWait_=0,spawnRetry_=0;
    bool seesPlayer_=false,hasGoal_=false;
    ThreatState state_=ThreatState::Dormant;
    std::vector<WorldPoint> path_;
    std::size_t pathIndex_=0,pathNodesVisited_=0;
    double random01();
    bool spawn(const World& world,const ThreatPlayer& player);
    bool playerCanSee(const World& world,const ThreatPlayer& player,double x,double z) const;
    void chooseWanderGoal(const World& world,const ThreatPlayer& player);
    bool plan(const World& world,double goalX,double goalZ);
    void walk(const World& world,float dt,float desiredSpeed);
};

} // namespace br
