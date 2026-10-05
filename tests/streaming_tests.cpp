#include "world.h"
#include "common/gameplay.h"
#include <stdexcept>
#include <cmath>
#include <iostream>

using namespace br;

static void check(bool value,const char* expression) {
    if (!value) throw std::runtime_error(expression);
}
#define CHECK(condition) check(bool(condition), #condition)

static void checkResident(const World& world, double x, double z) {
    const auto center=chunkAt(x,z);
    CHECK(world.chunks().size()==25);
    CHECK(world.preparedChunks().size()<=24);
    for (auto cz=center.z-2;cz<=center.z+2;++cz)
        for (auto cx=center.x-2;cx<=center.x+2;++cx)
            CHECK(world.chunks().count({cx,cz}));
    for (const auto& [coord,chunk]:world.preparedChunks()) {
        CHECK(!world.chunks().count(coord));
        CHECK(std::abs(coord.x-center.x)<=3 && std::abs(coord.z-center.z)<=3);
        CHECK(!chunk.vertices.empty());
        CHECK(chunk.maxHeight==static_cast<float>(ROOM_HEIGHT));
    }
}

int main() {
    // Match the game loop: one new CPU mesh per frame, even at sprint speed.
    // Cardinal and diagonal routes cross positive/negative chunk boundaries.
    for (const auto direction : {WorldPoint{1,0},WorldPoint{-1,0},WorldPoint{0,1},
                                  WorldPoint{0,-1},WorldPoint{.70710678,.70710678}}) {
        World world(12071998);
        double x=3,z=3;
        world.update(x,z);
        for (int frame=0;frame<1600;++frame) {
            x+=direction.x*SPRINT_SPEED/60;
            z+=direction.z*SPRINT_SPEED/60;
            world.update(x,z);
            world.prepareAhead(x,z);
            checkResident(world,x,z);
        }
        CHECK(world.streamPromotions()>0);
        CHECK(world.streamFallbacks()==0);
        // Turn back immediately: retained nearby geometry remains bounded.
        for (int frame=0;frame<600;++frame) {
            x-=direction.x*SPRINT_SPEED/60;
            z-=direction.z*SPRINT_SPEED/60;
            world.update(x,z);
            world.prepareAhead(x,z);
            checkResident(world,x,z);
        }
        CHECK(world.streamFallbacks()==0);
    }
    World world(12071998),reference(12071998);
    world.update(3,3);
    for (int i=0;i<24;++i) CHECK(world.prepareAhead(3,3));
    CHECK(!world.prepareAhead(3,3));
    world.update(CHUNK_SIZE+3,3);
    reference.update(CHUNK_SIZE+3,3);
    for (const auto& [coord,chunk]:world.chunks()) {
        const auto& expected=reference.chunks().at(coord);
        CHECK(chunk.vertices.size()==expected.vertices.size());
        CHECK(chunk.obstacles.size()==expected.obstacles.size());
        CHECK(chunk.lamps.size()==expected.lamps.size() && chunk.doors.size()==expected.doors.size());
        for (std::size_t i=0;i<chunk.lamps.size();++i) {
            CHECK(chunk.lamps[i].state==expected.lamps[i].state);
            CHECK(chunk.lamps[i].phase==expected.lamps[i].phase);
            CHECK(chunk.lamps[i].power==expected.lamps[i].power);
        }
        for (std::size_t i=0;i<chunk.doors.size();++i) {
            CHECK(chunk.doors[i].id==expected.doors[i].id);
            CHECK(chunk.doors[i].open==expected.doors[i].open);
        }
        for (std::size_t i=0;i<chunk.vertices.size();++i) {
            const auto& a=chunk.vertices[i];const auto& b=expected.vertices[i];
            CHECK(a.x==b.x && a.y==b.y && a.z==b.z && a.material==b.material);
        }
    }
    // Modified door geometry also survives demotion into the prepared ring,
    // promotion back, and a later animated toggle without rebuilding its static GPU mesh.
    world.update(3,3);
    DoorInfo door{};
    CHECK(world.nearestDoor(3,3,14,door));
    const auto owner=chunkAt(door.x-(door.east?.2:0),door.z-(door.east?0:.2));
    const double doorApproachX=door.x-(door.east?1:0),doorApproachZ=door.z+(door.east?0:1);
    CHECK(world.toggleDoor(door.id,doorApproachX,doorApproachZ));
    for (int i=0;i<100;++i) world.advanceDoors(.016f,doorApproachX,doorApproachZ);
    const auto openRevision=world.chunks().at(owner).revision;
    world.update(2*CHUNK_SIZE+3,3);
    CHECK(world.preparedChunks().count(owner));
    world.update(3,3);
    CHECK(world.chunks().at(owner).revision==openRevision);
    CHECK(!world.blocked(door.x,door.z,.25));
    CHECK(world.toggleDoor(door.id,doorApproachX,doorApproachZ));
    for (int i=0;i<100;++i) world.advanceDoors(.016f,doorApproachX,doorApproachZ);
    CHECK(world.chunks().at(owner).revision==openRevision);
    CHECK(world.blocked(door.x,door.z,.25));
    // A teleport still loads its full collision set synchronously; stale cache
    // cannot mix old layouts or seeds. Move assignment is the paused N/reset path.
    world.update(1e9+.5,-1e9+.5);
    checkResident(world,1e9+.5,-1e9+.5);
    world=World(42);
    CHECK(world.preparedChunks().empty() && world.chunks().empty());
    world.update(3,3);
    checkResident(world,3,3);
    CHECK(world.streamPromotions()==0 && world.streamFallbacks()==0);
    std::cout << "Streaming tests passed: bounded 25+24 cache, zero sprint/diagonal/turn-back generation misses, deterministic lamp and door promotion, animated-door cache persistence, teleport and seed reset.\n";
}
