#include "../world.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>

namespace {
void check(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

bool sameGeometry(const br::Chunk& a, const br::Chunk& b) {
    if (a.maxHeight!=b.maxHeight || a.vertices.size()!=b.vertices.size() || a.obstacles.size()!=b.obstacles.size()
        || a.overheads.size()!=b.overheads.size()
        || a.lamps.size()!=b.lamps.size()) return false;
    if (std::memcmp(a.vertices.data(),b.vertices.data(),a.vertices.size()*sizeof(br::Vertex))!=0) return false;
    for (std::size_t i=0; i<a.obstacles.size(); ++i) {
        const auto &one=a.obstacles[i], &two=b.obstacles[i];
        if (one.minX!=two.minX || one.minZ!=two.minZ || one.maxX!=two.maxX || one.maxZ!=two.maxZ) return false;
    }
    for (std::size_t i=0;i<a.overheads.size();++i) {
        const auto& one=a.overheads[i]; const auto& two=b.overheads[i];
        if(one.undersideY!=two.undersideY || one.footprint.minX!=two.footprint.minX
            || one.footprint.minZ!=two.footprint.minZ || one.footprint.maxX!=two.footprint.maxX
            || one.footprint.maxZ!=two.footprint.maxZ) return false;
    }
    for (std::size_t i=0; i<a.lamps.size(); ++i) {
        const auto &one=a.lamps[i], &two=b.lamps[i];
        if (one.x!=two.x || one.y!=two.y || one.z!=two.z || one.power!=two.power
            || one.r!=two.r || one.g!=two.g || one.b!=two.b || one.state!=two.state || one.phase!=two.phase) return false;
    }
    return true;
}

void variedRoomsAndExit() {
    constexpr std::uint64_t seed=42;
    br::World world(seed);
    world.update(3,3);
    std::set<int> widths,depths;
    std::set<br::RoomKind> kinds;
    std::set<br::FloorSurface> surfaces;
    for (const auto testSeed : {0ULL,42ULL,991ULL}) {
        for (int z=-36; z<=36; ++z) for (int x=-36; x<=36; ++x) {
            const auto info=br::roomInfo(x,z,testSeed);
            widths.insert(info.widthCells); depths.insert(info.depthCells);
            kinds.insert(info.kind);
            surfaces.insert(br::floorSurfaceAt(x*br::CELL+br::CELL/2,z*br::CELL+br::CELL/2,testSeed));
            check(br::roomKind(x,z,testSeed)==info.kind,"room kind uses the shared seeded partition");
            check(info.height==static_cast<float>(br::ROOM_HEIGHT),"one uniform 3.2 m ceiling");
            check(std::string(br::roomName(info.kind)).size()>0,"every room variant has a HUD name");
            if (info.kind==br::RoomKind::DeadEnd) {
                int openings=0;
                for (const auto step:{br::ChunkCoord{1,0},br::ChunkCoord{-1,0},br::ChunkCoord{0,1},br::ChunkCoord{0,-1}})
                    openings+=br::edgeOpen(x,z,x+step.x,z+step.z,testSeed,19)?1:0;
                check(openings==1,"dead-end room has exactly one permanent way in and out");
            }
            check(x>=info.x && x<info.x+info.widthCells && z>=info.z && z<info.z+info.depthCells,
                  "cell lies inside its room rectangle");
            check(br::floorDiv(info.x,br::CHUNK_CELLS)==br::floorDiv(info.x+info.widthCells-1,br::CHUNK_CELLS)
               && br::floorDiv(info.z,br::CHUNK_CELLS)==br::floorDiv(info.z+info.depthCells-1,br::CHUNK_CELLS),
                  "whole room fits in one chunk for merged floor and ceiling surfaces");
            for (int dz=0; dz<info.depthCells; ++dz) for (int dx=0; dx<info.widthCells; ++dx) {
                const auto other=br::roomInfo(info.x+dx,info.z+dz,testSeed);
                check(other.x==info.x && other.z==info.z && other.widthCells==info.widthCells
                      && other.depthCells==info.depthCells && other.height==info.height && other.kind==info.kind,
                      "every cell in a room shares its footprint and ceiling");
                for (std::uint64_t variant : {0ULL,4ULL,77ULL}) {
                    if (dx+1<info.widthCells)
                        check(br::edgeOpen(info.x+dx,info.z+dz,info.x+dx+1,info.z+dz,testSeed,variant),
                              "wide room internal east edge stays open under shifts");
                    if (dz+1<info.depthCells)
                        check(br::edgeOpen(info.x+dx,info.z+dz,info.x+dx,info.z+dz+1,testSeed,variant),
                              "deep room internal south edge stays open under shifts");
                }
            }
        }
        for (std::int64_t x : {-166666667LL,-1000LL,1000LL,166666667LL})
            for (std::int64_t z : {-166666667LL,-1000LL,1000LL,166666667LL}) {
                const auto info=br::roomInfo(x,z,testSeed);
                check(info.kind==br::roomKind(x,z,testSeed) && info.height==static_cast<float>(br::ROOM_HEIGHT),
                      "distant infinite generation retains stable room classification");
            }
    }
    check(kinds.size()==6,"all six seeded room variants occur");
    check(surfaces.size()==3,"carpet, damp carpet and tile occur");
    check(widths==std::set<int>{1,2,3} && depths==std::set<int>{1,2,3},
          "seeded layout has 7.5, 15 and 22.5 m room dimensions");
    check(!world.blocked(.8,.8,.25) && !world.blocked(4.5,.8,.25),"spawn has no furniture");
    check(!world.blocked(br::CELL,2,.25) && !world.blocked(2*br::CELL,2,.25),"22.5 m entrance room has fully open internal edges");
    for (int step=0; step<=720; ++step)
        check(!world.blocked(br::CELL/2+step*.1,br::CELL/2,.25),"guaranteed east route remains traversable");

    const std::set<int> allowedMaterials{0,1,2,3,5,7,16,18,19,32,33,34};
    std::set<int> materials;
    std::size_t vertexCount=0;
    bool hasUnlitFixture=false;
    for (const auto& [coord,chunk] : world.chunks()) {
        vertexCount+=chunk.vertices.size();
        check(chunk.maxHeight==static_cast<float>(br::ROOM_HEIGHT),"chunks all share the same ceiling height");
        check(chunk.lamps.size()>=br::CHUNK_CELLS*br::CHUNK_CELLS,"all ceiling fixtures have stable lamp metadata");
        for (const auto& lamp : chunk.lamps) {
            check(std::isfinite(lamp.power) && lamp.power>=0,"lamps have finite nonnegative power");
            hasUnlitFixture=hasUnlitFixture || lamp.state==br::LampState::Off;
            check((lamp.state==br::LampState::Off)==(lamp.power==0),"off lamps have zero power");
            check(lamp.phase>=0 && lamp.phase<1,"lamp phase remains normalized");
            check(lamp.y>0 && lamp.y<chunk.maxHeight,"fixtures remain below the ceiling");
            check(lamp.r>=0 && lamp.r<=1 && lamp.g>=0 && lamp.g<=1 && lamp.b>=0 && lamp.b<=1,
                  "lamp colors are valid RGB values");
        }
        for (const auto& vertex : chunk.vertices) {
            materials.insert(static_cast<int>(vertex.material));
            check(allowedMaterials.count(static_cast<int>(vertex.material))!=0,
                  "only yellow room surfaces, lights, clue and exit materials remain");
            if (vertex.material==16)
                check(vertex.u>=0 && vertex.u<=1 && vertex.v>=0 && vertex.v<=1,
                      "exit sign supplies normalized lettering UVs");
        }
        // Column footprints occupy grid intersections, leaving the clear
        // central walking cross and nearby cell corners unobstructed.
        for (int z=0; z<br::CHUNK_CELLS; ++z) for (int x=0; x<br::CHUNK_CELLS; ++x) {
            const auto gx=coord.x*br::CHUNK_CELLS+x,gz=coord.z*br::CHUNK_CELLS+z;
            if (gx==br::EXIT_CELL_X && gz==br::EXIT_CELL_Z) continue;
            for (double dx : {.6,br::CELL-.6}) for (double dz : {.6,br::CELL-.6})
                check(!world.blocked(gx*br::CELL+dx,gz*br::CELL+dz,.25),
                      "all room corners are empty of furniture and district props");
        }
    }
    auto startingMaterials=allowedMaterials; startingMaterials.erase(16);
    check(materials==startingMaterials,"classic surfaces, lamps and clues are generated without the distant exit sign at spawn");
    check(hasUnlitFixture,"some fluorescent fixtures remain unlit for atmosphere");
    check(vertexCount<500000,"varied world retains a bounded mesh budget");

    const auto exit=world.exitLocation();
    check(std::hypot(exit.x-br::CELL/2,exit.z-br::CELL/2)>260,
          "exit is a distant destination over 260 m from the player spawn");
    for (const auto& [coord,chunk]:world.chunks()) {
        (void)coord;
        for (const auto& door:chunk.doors)
            check(!door.isExit,"no exit remains in the initial resident window or its former location");
    }
    check(br::World(991).exitLocation().x==exit.x && br::World(991).exitLocation().z==exit.z,
          "exit landmark is stable across seeds");
    world.update(exit.x,exit.z);
    bool foundExitSign=false;
    for (const auto& [coord,chunk]:world.chunks()) {
        (void)coord;
        for (const auto& vertex:chunk.vertices) foundExitSign=foundExitSign || vertex.material==16;
    }
    check(foundExitSign,"the exit sign streams in at the distant landmark");
    check(!world.blocked(exit.x,exit.z,.25),"exit interaction threshold is clear");
    check(world.blocked(exit.x,exit.z-1.2,.25),"closed exit door has collision");
    for (double z=br::CELL/2;z>=exit.z;z-=.1) {
        world.update(br::CELL/2,z);
        check(!world.blocked(br::CELL/2,z,.25),"streamed north route from entrance to distant exit stays open");
    }
    world.update(exit.x,exit.z);
    for (int step=0; step<=50; ++step) {
        const double t=step/50.0;
        check(!world.blocked(br::CELL/2+(exit.x-br::CELL/2)*t,exit.z,.25),"exit approach is reachable");
    }
}

void doorsAndLampStates() {
    br::World world(12071998);
    world.update(3,3);
    br::DoorInfo nearby{};
    check(world.nearestDoor(3,3,14,nearby),"a discoverable optional door exists near the entrance");
    const auto originalDoor=nearby;
    const double px=nearby.x-(nearby.east?1:0),pz=nearby.z+(nearby.east?0:1);
    const auto owner=br::chunkAt(nearby.x-(nearby.east?.2:0),nearby.z-(nearby.east?0:.2));
    const auto before=world.chunks().at(owner).revision;
    check(world.blocked(nearby.x,nearby.z,.25),"closed door panel collides");
    check(!world.toggleDoor(nearby.id,px+10,pz+10),"distant door interaction is rejected");
    const auto closedVertices=world.doorVertices(0,0);
    check(world.toggleDoor(nearby.id,px,pz),"nearby door opens");
    check(world.blocked(nearby.x,nearby.z,.25),"opening is animated rather than instant");
    check(!world.advanceDoors(.3f,px,pz,.25,true),"pause freezes a door animation");
    check(world.nearestDoor(nearby.x,nearby.z,.1,nearby) && nearby.openness==0,"paused progress is unchanged");
    check(world.advanceDoors(.3f,px,pz),"hinge advances during play");
    check(world.nearestDoor(nearby.x,nearby.z,.1,nearby) && nearby.openness>0 && nearby.openness<1,
          "door reaches an intermediate hinge pose");
    const auto movingVertices=world.doorVertices(0,0);
    check(movingVertices.size()==closedVertices.size() &&
          std::memcmp(movingVertices.data(),closedVertices.data(),movingVertices.size()*sizeof(br::Vertex))!=0,
          "dynamic door triangles move without changing their count");
    const auto rebasedVertices=world.doorVertices(br::CHUNK_SIZE,-br::CHUNK_SIZE);
    for (std::size_t i=0;i<movingVertices.size();++i) {
        const auto& a=movingVertices[i];const auto& b=rebasedVertices[i];
        check(std::abs(a.x-b.x-br::CHUNK_SIZE)<.0001 && std::abs(a.z-b.z+br::CHUNK_SIZE)<.0001,
              "door vertices use the same floating render origin as rooms");
        check(a.nx==b.nx && a.ny==b.ny && a.nz==b.nz,"rebasing preserves hinge normals");
    }
    for (std::size_t i=0;i<movingVertices.size();i+=3) {
        const auto& a=movingVertices[i];const auto& b=movingVertices[i+1];const auto& c=movingVertices[i+2];
        const float ux=b.x-a.x,uy=b.y-a.y,uz=b.z-a.z,vx=c.x-a.x,vy=c.y-a.y,vz=c.z-a.z;
        check((uy*vz-uz*vy)*a.nx+(uz*vx-ux*vz)*a.ny+(ux*vy-uy*vx)*a.nz>0,
              "rotating a door preserves outward triangle winding");
    }
    for (int i=0;i<70;++i) world.advanceDoors(.016f,px,pz);
    check(world.chunks().at(owner).revision==before,"animation leaves the static room mesh untouched");
    check(!world.blocked(nearby.x,nearby.z,.25),"open door is traversable");
    check(!world.toggleDoor(nearby.id,nearby.x,nearby.z),"door cannot close onto player");
    check(!world.blocked(nearby.x,nearby.z,.25),"rejected close leaves collision open");
    world.update(3603,-3603);
    world.update(px,pz);
    check(world.nearestDoor(nearby.x,nearby.z,.1,nearby) && nearby.open,"open door survives unloading and return");
    check(world.toggleDoor(nearby.id,px,pz),"returned door closes");
    check(!world.blocked(nearby.x,nearby.z,.25),"closing starts from the current open pose");
    for (int i=0;i<70;++i) world.advanceDoors(.016f,px,pz);
    check(world.blocked(nearby.x,nearby.z,.25) && world.rememberedDoors()==0,"closing restores collision and removes override");
    check(world.toggleDoor(nearby.id,px,pz),"reopen first remembered door");

    std::set<br::LampState> states;
    bool hasPillar=false;
    for (const auto& [coord,chunk]:world.chunks()) {
        for (const auto& door:chunk.doors) {
            if (door.isExit) continue;
            const auto gx=static_cast<std::int64_t>(std::floor((door.x-(door.east?.1:0))/br::CELL));
            const auto gz=static_cast<std::int64_t>(std::floor((door.z-(door.east?0:.1))/br::CELL));
            check(!br::edgeReserved(gx,gz,gx+(door.east?1:0),gz+(door.east?0:1),world.seed()),
                  "interactable doors never occupy reserved routes");
        }
        for (const auto& lamp:chunk.lamps) {
            states.insert(lamp.state);
            for (float time:{0.0f,1.3f,17.9f,100.0f}) {
                const auto power=br::lampEnvelope(lamp.state,lamp.phase,time);
                check(power>=0 && power<=1.001f,"flicker power remains finite and bounded");
                if (lamp.state==br::LampState::Off) check(power==0,"off lamps stay off");
                if (lamp.state==br::LampState::Dying) check(power<.63f,"dying tubes never emit full power");
            }
        }
        for (const auto& vertex:chunk.vertices) if (vertex.material>=32) {
            const int packedState=static_cast<int>(vertex.material)-32;
            const float phase=vertex.material-std::floor(vertex.material);
            bool matchingLamp=false;
            for (const auto& lamp:chunk.lamps)
                if (static_cast<int>(lamp.state)==packedState && lamp.phase==phase &&
                    std::abs(lamp.x-vertex.x)<1 && std::abs(lamp.z-vertex.z)<.3f)
                    matchingLamp=true;
            check(matchingLamp,"tube mesh and emitted lamp share exact state and phase");
        }
        for (int z=0;z<br::CHUNK_CELLS;++z) for (int x=0;x<br::CHUNK_CELLS;++x) {
            const auto room=br::roomInfo(coord.x*br::CHUNK_CELLS+x,coord.z*br::CHUNK_CELLS+z,world.seed());
            if (room.kind==br::RoomKind::PillarHall) {
                hasPillar=true;
                check(world.blocked((room.x+1)*br::CELL,(room.z+1)*br::CELL,.25),"pillar hall has solid support columns");
            }
        }
    }
    check(states.size()==3 && hasPillar,"working, dying, off fixtures and pillar geometry all occur");

    // Open doors over widely separated chunks. Old overrides eventually expire,
    // while recent ones still reconstruct identically on a later streamed visit.
    int opened=1;
    br::DoorInfo last=nearby;
    for (int step=1;step<40 && opened<140;++step) {
        world.update(step*br::CHUNK_SIZE*5+3,3);
        std::vector<br::DoorInfo> candidates;
        for (const auto& [coord,chunk]:world.chunks())
            for (const auto& door:chunk.doors) if (!door.open) candidates.push_back(door);
        for (const auto& door:candidates) {
            check(world.toggleDoor(door.id,door.x-(door.east?1:0),door.z-(door.east?0:1)),"streamed optional door opens");
            last=door;
            ++opened;
            check(world.rememberedDoors()<=128,"door memory has a fixed bound");
            if (opened>=140) break;
        }
    }
    check(opened>=140 && world.rememberedDoors()==128,"bounded door retention is exercised");
    world.update(originalDoor.x,originalDoor.z);
    check(world.nearestDoor(originalDoor.x,originalDoor.z,.1,nearby) && !nearby.open,
          "oldest evicted door returns to seeded closed state");
    world.update(last.x,last.z);
    check(world.nearestDoor(last.x,last.z,.1,nearby) && nearby.open,
          "most recent door override survives streaming");
    std::cout<<"Nearby demo door: "<<originalDoor.x<<", "<<originalDoor.z
             <<"; approach from "<<px<<", "<<pz<<"\n";
}

void exitAndHingeSafety() {
    check(br::CELL==7.5 && br::CHUNK_SIZE==45,"rooms are 25 percent wider in both dimensions");
    for (std::uint64_t seed:{0ULL,1ULL,42ULL,991ULL,12071998ULL}) {
        br::World world(seed);
        const auto approach=world.exitLocation();
        world.update(approach.x,approach.z);
        br::DoorInfo exit{};
        check(world.nearestDoor(approach.x,approach.z,2,exit) && exit.isExit,"exit uses the normal interactable door API");
        check(!exit.east && exit.z==approach.z-1.2,"exit faces the south approach");
        check(world.blocked(exit.x,exit.z,.25),"closed leaf seals the real exit opening");
        check(!world.exitCrossed(exit.x,exit.z-.8),"a closed exit cannot finish the game from behind");
        check(!world.blocked(exit.x,exit.z-1,.25),"there is usable floor beyond the exit wall");
        check(world.blocked(exit.x-1.1,exit.z,.25) && world.blocked(exit.x+1.1,exit.z,.25),
              "full-height wall jambs flank the opening");
        check(world.blocked(br::CELL-2.55,exit.z-1,.25) && world.blocked(br::CELL,exit.z-1,.25)
              && world.blocked(exit.x,exit.z-3,.25),"side and back walls enclose an actual vestibule");
        for (double z=br::CELL/2;z>=exit.z-1;z-=.1) {
            world.update(br::CELL/2,z);
            check(!world.blocked(br::CELL/2,z,.25),"exit architecture preserves the streamed north route for every seed");
        }
        world.update(approach.x,approach.z);
        const auto chunkCoord=br::chunkAt(exit.x,exit.z);
        const auto& chunk=world.chunks().at(chunkCoord);
        const double localX=exit.x-chunkCoord.x*br::CHUNK_SIZE,localZ=exit.z-chunkCoord.z*br::CHUNK_SIZE;
        for (const auto& obstacle:chunk.obstacles)
            check(!(localX>=obstacle.minX && localX<=obstacle.maxX && localZ>=obstacle.minZ && localZ<=obstacle.maxZ),
                  "the exit aperture is cut through static collision rather than covering a solid wall");
        check(world.toggleDoor(exit.id,approach.x,approach.z),"exit starts opening with E interaction");
        world.advanceDoors(.3f,approach.x,approach.z);
        bool ownLeafOccludes=false;
        for (double along=.08;along<1.2-.35;along+=.08) {
            const double rayZ=exit.z-1.2+along;
            ownLeafOccludes=ownLeafOccludes || world.blocked(exit.x,rayZ,.23);
            check(!world.blocked(exit.x,rayZ,.23,exit.id),
                  "a partly open leaf cannot occlude its own interaction from behind");
        }
        check(ownLeafOccludes,"rear interaction regression actually crosses the animated panel");
        check(world.blocked(exit.x-1.1,exit.z,.23,exit.id),
              "ignoring the selected leaf still preserves solid wall occlusion");
        bool checkedOther=false;
        for (const auto& [coord,otherChunk]:world.chunks()) {
            (void)coord;
            for (const auto& other:otherChunk.doors) {
                if (other.id==exit.id || world.blocked(other.x,other.z,.23,other.id)) continue;
                check(world.blocked(other.x,other.z,.23,exit.id),
                      "ignoring the selected leaf leaves all other doors solid");
                checkedOther=true;
            }
        }
        check(checkedOther,"interaction occlusion tests at least one distinct door");
        for (int i=0;i<60;++i) world.advanceDoors(.016f,approach.x,approach.z);
        check(world.nearestDoor(exit.x,exit.z,.1,exit) && exit.openness==1,"exit reaches a fully open 90-degree hinge pose");
        for (int i=0;i<=25;++i)
            check(!world.blocked(exit.x,approach.z-i*.1,.25),"player can walk through the open wall aperture");
        check(!world.exitCrossed(exit.x,exit.z+.1) && world.exitCrossed(exit.x,exit.z-.8),
              "escape requires passing through the fully opened exit");
        check(!world.toggleDoor(exit.id,exit.x,exit.z),"exit refuses to close onto a player in its threshold");

        // Put the player in the middle of the closing sweep, away from both the
        // open and closed leaf. Collision must stop and reverse the moving door.
        const double hinge=exit.x-(.825-.035),width=2*(.825-.035);
        const double blockerX=hinge+width*.65*.70710678118;
        const double blockerZ=exit.z-width*.65*.70710678118;
        check(!world.blocked(blockerX,blockerZ,.25),"closing sweep test begins in empty space");
        check(world.toggleDoor(exit.id,approach.x,approach.z),"exit begins closing");
        bool reversed=false;
        for (int i=0;i<70;++i) {
            world.advanceDoors(.016f,blockerX,blockerZ);
            check(!world.blocked(blockerX,blockerZ,.25),"animated closing never sweeps through the player");
            world.nearestDoor(exit.x,exit.z,.1,exit);
            reversed=reversed || exit.open;
        }
        check(reversed && exit.openness==1,"blocked closing reverses back to a clear open position");
        check(world.toggleDoor(exit.id,approach.x,approach.z),"clear exit can close again");
        for (int i=0;i<60;++i) world.advanceDoors(.016f,approach.x,approach.z);
        check(world.blocked(exit.x,exit.z,.25) && !world.exitCrossed(exit.x,exit.z-.8),
              "closed exit restores panel collision and disables escape");
    }
}

void coordinates() {
    check(br::floorDiv(7,6)==1 && br::floorDiv(0,6)==0,"positive floor division");
    check(br::floorDiv(-1,6)==-1 && br::floorDiv(-6,6)==-1
          && br::floorDiv(-7,6)==-2,"negative floor division");
    check(br::chunkAt(0,0)==br::ChunkCoord{0,0},"origin chunk");
    check(br::chunkAt(-0.001,-br::CHUNK_SIZE)==br::ChunkCoord{-1,-1},"negative chunk boundary");
    check(br::chunkAt(-br::CHUNK_SIZE-.001,br::CHUNK_SIZE)==br::ChunkCoord{-2,1},"mixed chunk boundary");
    check(br::hashCell(-7,9,42)==br::hashCell(-7,9,42),"hash repeatability");
    check(br::hashCell(-7,9,42)!=br::hashCell(-7,9,43),"hash changes with seed");
}

void topology() {
    for (const std::uint64_t seed : {0ULL,1ULL,42ULL,1234567ULL}) {
        for (int z=-32; z<=32; ++z) for (int x=-32; x<=32; ++x) {
            for (const auto direction : {br::ChunkCoord{1,0},br::ChunkCoord{0,1}}) {
                const auto bx=x+direction.x,bz=z+direction.z;
                for (const auto variant : {0ULL,1ULL,19ULL}) {
                    const bool open=br::edgeOpen(x,z,bx,bz,seed,variant);
                    check(open==br::edgeOpen(bx,bz,x,z,seed,variant),"edge symmetry");
                    if (br::edgeReserved(x,z,bx,bz,seed)) check(open,"parent edge cannot close");
                    if (br::floorDiv(x,6)!=br::floorDiv(bx,6) || br::floorDiv(z,6)!=br::floorDiv(bz,6))
                        check(open==br::edgeOpen(x,z,bx,bz,seed,0),"variant cannot change chunk seams");
                }
            }
        }
        constexpr int extent=24;
        std::queue<br::ChunkCoord> pending;
        std::set<br::ChunkCoord> visited;
        pending.push({0,0}); visited.insert({0,0});
        while (!pending.empty()) {
            const auto here=pending.front(); pending.pop();
            for (const auto step : {br::ChunkCoord{1,0},br::ChunkCoord{-1,0},br::ChunkCoord{0,1},br::ChunkCoord{0,-1}}) {
                const br::ChunkCoord next{here.x+step.x,here.z+step.z};
                if (std::abs(next.x)>extent || std::abs(next.z)>extent) continue;
                if (!br::edgeReserved(here.x,here.z,next.x,next.z,seed)) continue;
                if (visited.insert(next).second) pending.push(next);
            }
        }
        check(visited.size()==(extent*2+1)*(extent*2+1),"permanent edges connect every tested cell to origin");
    }
}

void geometryAndCollision() {
    br::World world(42);
    world.update(3,3);
    check(world.chunks().size()==25,"default 5x5 resident window");
    check(!world.blocked(3,3,.25),"spawn is clear");
    check(!world.blocked(3,0,.25),"spawn north doorway is clear");
    check(world.blocked(.94,0,.25),"spawn north doorway jamb collides");
    bool hasClue=false,hasLamps=false;
    for (const auto& [coord,chunk] : world.chunks()) {
        check(chunk.vertices.size()%3==0,"triangulated mesh");
        double floorArea=0,ceilingArea=0;
        for (std::size_t i=0; i<chunk.vertices.size(); i+=3) {
            const auto &a=chunk.vertices[i],&b=chunk.vertices[i+1],&c=chunk.vertices[i+2];
            const double area=std::abs((b.x-a.x)*(c.z-a.z)-(b.z-a.z)*(c.x-a.x))*.5;
            if (a.material==1 || a.material==18 || a.material==19) floorArea+=area;
            if (a.material==2) ceilingArea+=area;
        }
        check(floorArea==br::CHUNK_SIZE*br::CHUNK_SIZE && ceilingArea==floorArea,
              "merged floors and ceilings cover exactly one whole chunk");
        hasLamps=hasLamps || !chunk.lamps.empty();
        for (const auto& vertex : chunk.vertices) {
            check(vertex.x>=-.14f && vertex.x<=br::CHUNK_SIZE+.14f && vertex.z>=-.14f && vertex.z<=br::CHUNK_SIZE+.14f,
                  "mesh uses chunk-local coordinates");
            check(vertex.y>=0 && vertex.y<=chunk.maxHeight+.001f,"per-chunk room height bounds");
            hasClue=hasClue || vertex.material==5;
        }
        for (std::size_t i=0; i<chunk.vertices.size(); i+=3) {
            const auto &a=chunk.vertices[i], &b=chunk.vertices[i+1], &c=chunk.vertices[i+2];
            const float ux=b.x-a.x,uy=b.y-a.y,uz=b.z-a.z;
            const float vx=c.x-a.x,vy=c.y-a.y,vz=c.z-a.z;
            const float facing=(uy*vz-uz*vy)*a.nx+(uz*vx-ux*vz)*a.ny+(ux*vy-uy*vx)*a.nz;
            check(facing>0,"triangle winding matches outward normal");
        }
    }
    check(hasClue && hasLamps,"clues and fixtures are generated");
    for (int z=-10; z<=10; ++z) for (int x=-10; x<=10; ++x) {
        check(!world.blocked(x*br::CELL+br::CELL/2,z*br::CELL+br::CELL/2,.25),"all cell centres traversable");
        for (const auto direction : {br::ChunkCoord{1,0},br::ChunkCoord{0,1}}) {
            const bool open=br::edgeOpen(x,z,x+direction.x,z+direction.z,42);
            const double edgeX=x*br::CELL+br::CELL/2+direction.x*br::CELL/2;
            const double edgeZ=z*br::CELL+br::CELL/2+direction.z*br::CELL/2;
            br::DoorInfo door{};
            const bool closedDoor=world.nearestDoor(edgeX,edgeZ,.1,door) && !door.open;
            for (const double offset : {-0.2,0.0,0.2})
                check(world.blocked(edgeX+direction.x*offset,edgeZ+direction.z*offset,.25)==(!open || closedDoor),
                      "actual doorway collision agrees on both sides, including negative seams");
            if (open && !closedDoor) {
                // Full centre-to-centre path checks frame gaps and decorative props.
                for (int step=-37; step<=37; ++step)
                    check(!world.blocked(edgeX+direction.x*step*.1,edgeZ+direction.z*step*.1,.25),
                          "open edge has an unobstructed centre route");
            }
            const auto here=br::roomInfo(x,z,42),next=br::roomInfo(x+direction.x,z+direction.z,42);
            if (here.x==next.x && here.z==next.z) {
                for (double across : {-2.0,0.0,2.0})
                    check(!world.blocked(edgeX+direction.z*across,edgeZ+direction.x*across,.25),
                          "shared room edge is open across its full width, beyond a door-sized gap");
            }

        }
    }
    check(world.blocked(200,200,.25),"non-resident location blocks");
    check(world.blocked(3*br::CHUNK_SIZE-.1,3,.25),"player cannot partly enter missing neighbour");
    check(world.blocked(3,3,-.1),"invalid player radius blocks");
}

void streamingAndShifts() {
    br::World first(851),second(851),otherSeed(852);
    first.update(3,3); second.update(3,3); otherSeed.update(3,3);
    bool differentSeed=false;
    for (const auto& [coord,chunk] : first.chunks()) {
        check(sameGeometry(chunk,second.chunks().at(coord)),"same seed reproduces geometry");
        differentSeed=differentSeed || !sameGeometry(chunk,otherSeed.chunks().at(coord));
    }
    check(differentSeed,"seed changes layout");
    const auto original=first.chunks();
    check(first.shiftBehind(3,3,0,-1),"distant hidden chunk can shift");
    check(first.shifts()==1,"shift counter");
    int changed=0;
    for (const auto& [coord,chunk] : first.chunks()) {
        if (chunk.revision==original.at(coord).revision) continue;
        ++changed;
        const double dx=(coord.x+.5)*br::CHUNK_SIZE-3,dz=(coord.z+.5)*br::CHUNK_SIZE-3;
        constexpr double bound=br::CHUNK_SIZE*.7071067811865475244+.25;
        check(std::hypot(dx,dz)-bound>45,"shift remains beyond exclusion distance");
        check(-dz+bound < -6,"entire shifted chunk lies behind view plane");
        // Border obstacle geometry remains identical after an internal-only shift.
        auto borderObstacles=[](const br::Chunk& c) {
            std::vector<br::Aabb> result;
            for (const auto& b : c.obstacles)
                if (b.maxX>br::CHUNK_SIZE || b.maxZ>br::CHUNK_SIZE) result.push_back(b);
            return result;
        };
        const auto before=borderObstacles(original.at(coord)),after=borderObstacles(chunk);
        check(before.size()==after.size(),"shift keeps boundary wall count");
        for (std::size_t i=0; i<before.size(); ++i)
            check(before[i].minX==after[i].minX && before[i].minZ==after[i].minZ
                  && before[i].maxX==after[i].maxX && before[i].maxZ==after[i].maxZ,
                  "shift keeps boundary collision geometry");
    }
    check(changed==1,"one chunk changes per shift");
    check(sameGeometry(first.chunks().at({0,0}),original.at({0,0})),"shift leaves player chunk intact");
    check(!first.shiftBehind(3,3,0,0),"invalid view direction cannot shift");
    for (int step=1; step<=30; ++step) {
        first.update(step*br::CHUNK_SIZE+3,-step*br::CHUNK_SIZE+3);
        check(first.chunks().size()==25,"streaming keeps bounded chunk count");
    }
    first.update(3,3);
    for (const auto& [coord,chunk] : first.chunks())
        check(sameGeometry(chunk,original.at(coord)),"unloaded shifts reset to deterministic seed layout");
    first.update(3,3,1);
    check(first.chunks().size()==9,"smaller radius evicts unnecessary chunks");
    first.update(3,3,0);
    check(first.chunks().size()==1,"single-chunk resident window");
    check(!first.shiftBehind(3,3,0,-1),"near chunks cannot shift");
}

void actorProtectionAndDoorBroadphase() {
    br::World shifted(851); shifted.update(3,3);
    // The original deterministic choice is a real eligible chunk, so protecting
    // its actor must change that choice or postpone the shift entirely.
    br::World reference(851); reference.update(3,3);
    const auto original=reference.chunks();
    check(reference.shiftBehind(3,3,0,-1),"unprotected reference has an eligible distant shift");
    br::ChunkCoord protectedChunk{}; bool selected=false;
    for (const auto& [coord,chunk]:reference.chunks()) if (chunk.revision!=original.at(coord).revision) {
        protectedChunk=coord; selected=true; break;
    }
    check(selected,"reference exposes the chunk that would otherwise change");
    const br::WorldPoint actor{(protectedChunk.x+.5)*br::CHUNK_SIZE,(protectedChunk.z+.5)*br::CHUNK_SIZE};
    for(int attempt=0;attempt<8;++attempt) shifted.shiftBehind(3,3,0,-1,actor);
    for (const auto& [coord,chunk]:shifted.chunks())
        if (std::abs(coord.x-protectedChunk.x)<=1 && std::abs(coord.z-protectedChunk.z)<=1)
            check(chunk.revision==original.at(coord).revision,"entity chunk and all neighbours remain unchanged by repeated hidden shifts");

    br::World world(42); const auto approach=world.exitLocation(); world.update(approach.x,approach.z);
    br::DoorInfo exit{}; check(world.nearestDoor(approach.x,approach.z,2,exit),"secondary actor door fixture is present");
    check(world.toggleDoor(exit.id,approach.x,approach.z),"secondary actor fixture opens");
    for(int i=0;i<60;++i) world.advanceDoors(.016f,approach.x,approach.z);
    const double hinge=exit.x-(.825-.035),width=2*(.825-.035);
    const br::WorldPoint blocker{hinge+width*.65*.70710678118,exit.z-width*.65*.70710678118};
    check(!world.blocked(blocker.x,blocker.z,.26),"secondary actor starts clear of open leaf");
    check(world.toggleDoor(exit.id,approach.x,approach.z),"player starts door closing with entity in swing area");
    bool reversed=false;
    for(int i=0;i<70;++i) {
        world.advanceDoors(.016f,approach.x,approach.z,.23,false,blocker,.26);
        check(!world.blocked(blocker.x,blocker.z,.26),"closing panel never sweeps through secondary actor capsule");
        world.nearestDoor(exit.x,exit.z,.1,exit); reversed=reversed || exit.open;
    }
    check(reversed && exit.openness==1,"closing leaf reverses for an entity as it does for a player");

    // Regression for a full-open east leaf protruding past its owner's chunk
    // border. Derive sample points from rendered geometry, not collider math.
    br::World seams(42); seams.update(3.75,3.75);
    std::vector<br::DoorInfo> chosen;
    for(const auto& [coord,chunk]:seams.chunks()) for(const auto& door:chunk.doors)
        if(door.east && std::abs(door.x/br::CHUNK_SIZE-std::round(door.x/br::CHUNK_SIZE))<1e-8)
            chosen.push_back(door);
    check(chosen.size()>=2,"seeded fixture includes swinging leaves on chunk seams");
    for(const auto& door:chosen) check(seams.toggleDoor(door.id,door.x-1,door.z),"seam door starts opening");
    int verified=0; bool negative=false,positive=false;
    for(int phase=0;phase<5;++phase) {
        if(phase) seams.advanceDoors(.2f,10000,10000);
        const auto vertices=seams.doorVertices(0,0);
        std::size_t offset=0;
        for(const auto& [coord,chunk]:seams.chunks()) for(const auto& door:chunk.doors) {
            const bool seam=door.east && std::abs(door.x/br::CHUNK_SIZE-std::round(door.x/br::CHUNK_SIZE))<1e-8;
            if(seam) {
                const double x=(vertices[offset].x+vertices[offset+1].x)*.5;
                const double z=(vertices[offset].z+vertices[offset+1].z)*.5;
                if(!seams.blocked(x,z,.26,door.id)) {
                    check(seams.blocked(x,z,.26),"rendered moving leaf retains collision across ownership seams");
                    ++verified; negative=negative || coord.x<0 || coord.z<0; positive=positive || coord.x>=0;
                }
            }
            offset+=72; // One36-vertex panel and one36-vertex handle per leaf.
        }
    }
    check(verified>=10 && negative && positive,"seam collision regression covers multiple poses and both coordinate signs");
}

void actualOverheadClearance() {
    br::World world(42); world.update(3.75,3.75);
    const auto close=[](float one,float two) {return std::abs(one-two)<.0001f;};
    check(close(world.headClearance(1.5,1.5,.25),3.2f),"empty floor retains the actual3.2m ceiling height");
    check(close(world.headClearance(3.75,3.75,.15),3.049f),"ceiling fixture underside reduces clearance using its rendered geometry");
    check(close(world.headClearance(.9,0,.15),3.2f),"solid full-height wall is not mistaken for a zero-height overhead");
    check(world.headClearance(10000,10000,1)==0 && world.headClearance(3.75,3.75,-1)==0,
          "unloaded or invalid clearance queries fail closed");
    br::DoorInfo door{};
    check(world.nearestDoor(3,3,14,door),"head-clearance doorway fixture exists");
    check(close(world.headClearance(door.x,door.z,.26),2.40f),"normal doorframe header has actual2.40m underside");
    const double nx=door.east?1:0,nz=door.east?0:1;
    check(close(world.headClearance(door.x+nx*.8,door.z+nz*.8,1),2.40f),"wide query anticipates the door before the actor's head reaches it");
    check(world.headClearance(door.x+nx*.8,door.z+nz*.8,.26)>2.85f,"smaller footprint distinguishes nearby header from current room height");
    check(world.toggleDoor(door.id,door.x+nx,door.z+nz),"clearance fixture door opens");
    for(int i=0;i<60;++i) world.advanceDoors(.016f,door.x+nx,door.z+nz);
    check(close(world.headClearance(door.x,door.z,.26),2.40f),"opening the leaf leaves its fixed frame overhead in place");

    bool wide=false,standard=false,negativeSeam=false;
    for(const auto& [coord,chunk]:world.chunks()) {
        check(!chunk.overheads.empty(),"streamed chunks cache ceilings and raised geometry");
        for(const auto& overhead:chunk.overheads) {
            check(overhead.undersideY>=2 && overhead.undersideY<=br::ROOM_HEIGHT+1e-5,
                  "cache contains raised geometry only");
            const auto& box=overhead.footprint;
            const double x=coord.x*br::CHUNK_SIZE+(box.minX+box.maxX)*.5;
            const double z=coord.z*br::CHUNK_SIZE+(box.minZ+box.maxZ)*.5;
            br::DoorInfo framed{};
            if(close(overhead.undersideY,2.86f) && !world.blocked(x,z,.26)) {
                check(close(world.headClearance(x,z,.26),2.86f),"wide hallway lintel reports its actual2.86m underside"); wide=true;
            }
            if(close(overhead.undersideY,2.48f) && !world.blocked(x,z,.26)
                && !world.nearestDoor(x,z,.1,framed)) {
                check(close(world.headClearance(x,z,.26),2.48f),"unframed standard doorway reports its actual2.48m underside"); standard=true;
            }
        }
        for(const auto& candidate:chunk.doors)
            if((coord.x<0 || coord.z<0) && candidate.east
                && std::abs(candidate.x/br::CHUNK_SIZE-std::round(candidate.x/br::CHUNK_SIZE))<1e-8
                && world.headClearance(candidate.x+.2,candidate.z,.26)>0) {
                check(close(world.headClearance(candidate.x+.2,candidate.z,.26),2.40f),
                      "overhead query finds neighboring owner's frame across a negative chunk boundary"); negativeSeam=true;
            }
    }
    check(wide && standard && negativeSeam,"clearance fixtures exercise wide, standard and negative-boundary headers");
}
}

int main() {
    try {
        coordinates(); topology(); geometryAndCollision(); variedRoomsAndExit(); streamingAndShifts(); doorsAndLampStates(); exitAndHingeSafety(); actorProtectionAndDoorBroadphase(); actualOverheadClearance();
        std::cout << "PASS: coordinates, seeded topology, connectivity, mesh winding, six room variants, seeded surfaces, merged surface coverage, room dimensions, uniform ceilings, lights, exit approach, streaming, and safe shifts\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
