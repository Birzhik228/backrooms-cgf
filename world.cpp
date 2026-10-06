#include "world.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace br {
namespace {
constexpr double WALL_HALF = 0.10;
constexpr double DOOR_HALF = 1.13;
constexpr double DOOR_TOP = 2.48;
constexpr double EXIT_HALF = .825;
constexpr double DOOR_THICKNESS = .045;

std::uint64_t mix(std::uint64_t value) {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

ChunkCoord parent(std::int64_t x, std::int64_t z, std::uint64_t seed) {
    if (x == 0 && z == 0) return {0,0};
    if (x != 0 && (z == 0 || (hashCell(x,z,seed) & 1)))
        return {x + (x > 0 ? -1 : 1), z};
    return {x, z + (z > 0 ? -1 : 1)};
}

bool adjacent(std::int64_t x, std::int64_t z, std::int64_t bx, std::int64_t bz) {
    return (x == bx && std::abs(z-bz) == 1) || (z == bz && std::abs(x-bx) == 1);
}

std::uint64_t edgeHash(std::int64_t x, std::int64_t z,
                       std::int64_t bx, std::int64_t bz, std::uint64_t seed) {
    // Canonical lower endpoint and orientation make this independent of caller.
    const auto lowX = std::min(x,bx), lowZ = std::min(z,bz);
    return hashCell(lowX,lowZ,seed ^ (x != bx ? 0x718454a3d921ULL : 0xc37b64ab571eULL));
}

struct P { float x,y,z; };

void quad(Chunk& c, P a, P b, P d, P e, P normal,
          float width, float height, float material) {
    const Vertex points[4] = {
        {a.x,a.y,a.z,normal.x,normal.y,normal.z,0,0,material},
        {b.x,b.y,b.z,normal.x,normal.y,normal.z,width,0,material},
        {d.x,d.y,d.z,normal.x,normal.y,normal.z,width,height,material},
        {e.x,e.y,e.z,normal.x,normal.y,normal.z,0,height,material}
    };
    for (int index : {0,1,2,0,2,3}) c.vertices.push_back(points[index]);
}

void box(Chunk& c, float x0, float y0, float z0,
         float x1, float y1, float z1, float material, bool collision = false) {
    const float w=x1-x0,h=y1-y0,d=z1-z0;
    if (y0>=2.0f && h>0 && w>0 && d>0)
        c.overheads.push_back({{x0,z0,x1,z1},y0});
    quad(c,{x0,y0,z1},{x1,y0,z1},{x1,y1,z1},{x0,y1,z1},{0,0,1},w,h,material);
    quad(c,{x1,y0,z0},{x0,y0,z0},{x0,y1,z0},{x1,y1,z0},{0,0,-1},w,h,material);
    quad(c,{x1,y0,z1},{x1,y0,z0},{x1,y1,z0},{x1,y1,z1},{1,0,0},d,h,material);
    quad(c,{x0,y0,z0},{x0,y0,z1},{x0,y1,z1},{x0,y1,z0},{-1,0,0},d,h,material);
    quad(c,{x0,y1,z1},{x1,y1,z1},{x1,y1,z0},{x0,y1,z0},{0,1,0},w,d,material);
    quad(c,{x0,y0,z0},{x1,y0,z0},{x1,y0,z1},{x0,y0,z1},{0,-1,0},w,d,material);
    if (collision) c.obstacles.push_back({x0,z0,x1,z1});
}

void wallSegment(Chunk& c, bool east, float edge, float lo, float hi,
                 float bottom, float top, bool collision) {
    if (hi <= lo || top<=bottom) return;
    const float half = static_cast<float>(WALL_HALF);
    if (east) box(c,edge-half,bottom,lo,edge+half,top,hi,0,collision);
    else box(c,lo,bottom,edge-half,hi,top,edge+half,0,collision);
    if (bottom == 0) {
        const float trim=half+0.014f;
        if (east) box(c,edge-trim,0,lo,edge+trim,0.12f,hi,3);
        else box(c,lo,0,edge-trim,hi,0.12f,edge+trim,3);
    }
}

void wall(Chunk& c, bool east, float edge, float begin, bool open, bool wide) {
    const float end=begin+static_cast<float>(CELL);
    const float middle=begin+static_cast<float>(CELL/2);
    constexpr float top=static_cast<float>(ROOM_HEIGHT);
    if (!open) {
        wallSegment(c,east,edge,begin,end,0,top,true);
        return;
    }
    const float half = wide ? 2.45f : static_cast<float>(DOOR_HALF);
    wallSegment(c,east,edge,begin,middle-half,0,top,true);
    wallSegment(c,east,edge,middle+half,end,0,top,true);
    wallSegment(c,east,edge,middle-half,middle+half,
                wide ? 2.86f : static_cast<float>(DOOR_TOP),top,false);
}

bool sharedHall(std::int64_t x, std::int64_t z, std::int64_t bx, std::int64_t bz,
                std::uint64_t seed) {
    const auto one=roomInfo(x,z,seed),two=roomInfo(bx,bz,seed);
    return one.x==two.x && one.z==two.z;
}

void ceilingLight(Chunk& c, float x, float z, float power, LampState state,float phase) {
    constexpr float ceiling=static_cast<float>(ROOM_HEIGHT);
    box(c,x-1.04f,ceiling-.14f,z-.28f,x+1.04f,ceiling-.015f,z+.28f,7);
    // Integer part identifies the state; the fractional part carries the same
    // stable quantized phase that the CPU uses for the actual emitted light.
    const float material=32.0f+static_cast<float>(state)+phase;
    box(c,x-.92f,ceiling-.151f,z-.185f,x+.92f,ceiling-.14f,z-.055f,material);
    box(c,x-.92f,ceiling-.151f,z+.055f,x+.92f,ceiling-.14f,z+.185f,material);
    c.lamps.push_back({x,ceiling-.22f,z,state==LampState::Off?0.0f:power,
                       1.0f,.965f,.77f,state,phase});
}

void doorFrame(Chunk& c,float edge,float middle,bool east,float half) {
    auto panel=[&](float lo,float hi,float bottom,float top,float thickness,float material,bool collision) {
        if (east) box(c,edge-thickness,bottom,lo,edge+thickness,top,hi,material,collision);
        else box(c,lo,bottom,edge-thickness,hi,top,edge+thickness,material,collision);
    };
    panel(middle-half-.055f,middle-half+.045f,0,2.49f,.125f,7,true);
    panel(middle+half-.045f,middle+half+.055f,0,2.49f,.125f,7,true);
    panel(middle-half,middle+half,2.40f,2.49f,.125f,7,false);
}

void exitDoor(Chunk& c, float x, float z) {
    // A full-height vestibule is built into the northeast corner, with a real
    // opening through its front wall. The side/back walls enclose usable floor
    // behind the leaf; the central north/south route stays outside this alcove.
    const float left=x+static_cast<float>(CELL)-2.55f,right=x+static_cast<float>(CELL);
    const float cx=(left+right)*.5f,front=z+3.0f,back=z;
    constexpr float half=static_cast<float>(EXIT_HALF),top=static_cast<float>(ROOM_HEIGHT);
    wallSegment(c,false,front,left,cx-half,0,top,true);
    wallSegment(c,false,front,cx+half,right,0,top,true);
    wallSegment(c,false,front,cx-half,cx+half,static_cast<float>(DOOR_TOP),top,false);
    wallSegment(c,true,left,back,front,0,top,true);
    wallSegment(c,true,right,back,front,0,top,true);
    wallSegment(c,false,back,left,right,0,top,true);
    doorFrame(c,front,cx,false,half);
    box(c,cx-.65f,2.72f,front-.03f,cx+.65f,3.08f,front+.09f,3);
    quad(c,{cx-.60f,2.75f,front+.092f},{cx+.60f,2.75f,front+.092f},
           {cx+.60f,3.05f,front+.092f},{cx-.60f,3.05f,front+.092f},{0,0,1},1,1,16);
    c.lamps.push_back({cx,2.67f,front+.45f,1.18f,.38f,1.0f,.57f});
}

void clue(Chunk& c, float centerX, float centerZ, bool east, float normalSign) {
    constexpr float halfWidth=0.48f, bottom=1.12f, top=1.95f;
    // UVs 0..1 are used by the material-5 flashlight-only arrow shader.
    if (east && normalSign > 0)
        quad(c,{centerX,bottom,centerZ+halfWidth},{centerX,bottom,centerZ-halfWidth},
               {centerX,top,centerZ-halfWidth},{centerX,top,centerZ+halfWidth},{1,0,0},1,1,5);
    else if (east)
        quad(c,{centerX,bottom,centerZ-halfWidth},{centerX,bottom,centerZ+halfWidth},
               {centerX,top,centerZ+halfWidth},{centerX,top,centerZ-halfWidth},{-1,0,0},1,1,5);
    else if (normalSign > 0)
        quad(c,{centerX-halfWidth,bottom,centerZ},{centerX+halfWidth,bottom,centerZ},
               {centerX+halfWidth,top,centerZ},{centerX-halfWidth,top,centerZ},{0,0,1},1,1,5);
    else
        quad(c,{centerX+halfWidth,bottom,centerZ},{centerX-halfWidth,bottom,centerZ},
               {centerX-halfWidth,top,centerZ},{centerX+halfWidth,top,centerZ},{0,0,-1},1,1,5);
}

bool circleHits(const Aabb& b, double x, double z, double radius) {
    const double dx=x-std::clamp(x,b.minX,b.maxX);
    const double dz=z-std::clamp(z,b.minZ,b.maxZ);
    return dx*dx+dz*dz < radius*radius;
}

struct DoorPose { double hingeX,hingeZ,alongX,alongZ,width; };

DoorPose doorPose(const DoorInfo& door,float openness) {
    const double half=(door.isExit?EXIT_HALF:DOOR_HALF)-.035;
    // Ease into and out of a 90-degree swing, keeping collision and the rendered
    // leaf on exactly the same transform. South-facing exits swing inward.
    const double t=std::clamp(static_cast<double>(openness),0.0,1.0);
    const double angle=t*t*(3-2*t)*1.57079632679489661923;
    const double alongX=door.east?std::sin(angle):std::cos(angle);
    const double alongZ=door.east?std::cos(angle):-std::sin(angle);
    return {door.x-(door.east?0:half),door.z-(door.east?half:0),alongX,alongZ,2*half};
}

bool doorHits(const DoorInfo& door,float openness,double x,double z,double radius) {
    // Most navigation samples are nowhere near a leaf. Reject its complete
    // 90-degree sweep before calculating trigonometric hinge transforms.
    const double reach=2*((door.isExit?EXIT_HALF:DOOR_HALF)-.035)+DOOR_THICKNESS+radius;
    if (std::abs(x-door.x)>reach || std::abs(z-door.z)>reach) return false;
    const auto pose=doorPose(door,openness);
    const double dx=x-pose.hingeX,dz=z-pose.hingeZ;
    const double bound=pose.width+radius+.10;
    if (dx*dx+dz*dz>bound*bound) return false;
    const double along=dx*pose.alongX+dz*pose.alongZ;
    const double across=dx*pose.alongZ-dz*pose.alongX;
    return circleHits({0,-DOOR_THICKNESS,pose.width,DOOR_THICKNESS},along,across,radius);
}
} // namespace

std::int64_t floorDiv(std::int64_t value, std::int64_t divisor) {
    if (divisor <= 0) throw std::invalid_argument("floorDiv needs a positive divisor");
    const auto quotient=value/divisor, remainder=value%divisor;
    return quotient-(remainder < 0 ? 1 : 0);
}

ChunkCoord chunkAt(double x, double z) {
    return {static_cast<std::int64_t>(std::floor(x/CHUNK_SIZE)),
            static_cast<std::int64_t>(std::floor(z/CHUNK_SIZE))};
}

std::uint64_t hashCell(std::int64_t x, std::int64_t z, std::uint64_t seed) {
    return mix(mix(static_cast<std::uint64_t>(x) ^ seed)
               ^ mix(static_cast<std::uint64_t>(z) + 0x632be59bd9b4e019ULL));
}

RoomKind roomKind(std::int64_t x, std::int64_t z, std::uint64_t seed) {
    return roomInfo(x,z,seed).kind;
}

RoomInfo roomInfo(std::int64_t x, std::int64_t z, std::uint64_t seed) {
    // A small rectangular partition gives actual room footprints, independent
    // of chunk loading. Every cell in a rectangle shares one ceiling height.
    const auto tileX=floorDiv(x,3)*3,tileZ=floorDiv(z,3)*3;
    const int lx=static_cast<int>(x-tileX),lz=static_cast<int>(z-tileZ);
    const auto h=hashCell(tileX,tileZ,seed^0x916f2938cULL);
    const int layout=(tileX==0 && tileZ==0) ? 2 : static_cast<int>(h%8);
    int ox=0,oz=0,width=3,depth=3;
    switch (layout) {
    case 0: oz=lz; depth=1; break; // Three-cell galleries.
    case 1: ox=lx; width=1; break; // Three-cell narrow halls.
    case 2: case 7:
        if (lz<2) depth=2;        // Three by two cells plus smaller rooms.
        else { oz=2; depth=1; ox=lx==0 ? 0 : 1; width=lx==0 ? 1 : 2; }
        break;
    case 3:
        if (lx<2) width=2;
        else { ox=2; width=1; oz=lz==0 ? 0 : 1; depth=lz==0 ? 1 : 2; }
        break;
    case 4:
        if (lx<2 && lz<2) { width=2; depth=2; }
        else if (lx==2) { ox=2; width=1; }
        else { oz=2; width=2; depth=1; }
        break;
    case 6: break; // Unbroken three by three-cell hall.
    default:
        ox=lx; oz=lz; width=1; depth=1;
        break;
    }
    const auto roomX=tileX+ox,roomZ=tileZ+oz;
    const auto flavor=hashCell(roomX,roomZ,seed^0x45b924aeULL);
    RoomKind kind=RoomKind::Classic;
    if (width>=2 && depth>=2)
        kind=(flavor%3==0)?RoomKind::PillarHall:RoomKind::OpenHall;
    else if (width>1 || depth>1) kind=RoomKind::Corridor;
    else {
        int routes=0;
        for (const auto step:{ChunkCoord{1,0},ChunkCoord{-1,0},ChunkCoord{0,1},ChunkCoord{0,-1}})
            routes+=edgeReserved(roomX,roomZ,roomX+step.x,roomZ+step.z,seed)?1:0;
        if (routes==1 && flavor%3!=0) kind=RoomKind::DeadEnd;
    }
    if (flavor%13==0 && kind!=RoomKind::DeadEnd) kind=RoomKind::OddRoom;
    // The entrance and exit approach remain uncluttered familiar yellow rooms.
    if ((roomX==0 && roomZ==0) || (roomX<=EXIT_CELL_X && roomX+width>EXIT_CELL_X
                               && roomZ<=EXIT_CELL_Z && roomZ+depth>EXIT_CELL_Z))
        kind=RoomKind::Classic;
    return {roomX,roomZ,width,depth,static_cast<float>(ROOM_HEIGHT),kind};
}

const char* roomName(RoomKind kind) {
    switch (kind) {
    case RoomKind::Corridor: return "Repeating corridor";
    case RoomKind::OpenHall: return "Open hall";
    case RoomKind::PillarHall: return "Pillar hall";
    case RoomKind::DeadEnd: return "Dead end";
    case RoomKind::OddRoom: return "Tiled room";
    default: return "Yellow halls";
    }
}

FloorSurface floorSurfaceAt(double x,double z,std::uint64_t seed) {
    const auto room=roomInfo(static_cast<std::int64_t>(std::floor(x/CELL)),
                             static_cast<std::int64_t>(std::floor(z/CELL)),seed);
    if (room.kind==RoomKind::OddRoom) return FloorSurface::Tile;
    return hashCell(room.x,room.z,seed^0x724ac329ULL)%6==0
        ?FloorSurface::DampCarpet:FloorSurface::Carpet;
}

float lampEnvelope(LampState state,float phase,float time) {
    if (state==LampState::Off) return 0;
    const float p=phase*6.28318530718f;
    if (state==LampState::Working)
        return .98f+.014f*std::sin(time*3.1f+p)+.006f*std::sin(time*7.7f+p*2.3f);
    const float t=std::clamp((std::sin(time*2.7f+p)+.2f)/.65f,0.0f,1.0f);
    return (.12f+.50f*t*t*(3-2*t))*(.94f+.06f*std::sin(time*11+p*2.3f));
}

bool edgeReserved(std::int64_t x, std::int64_t z,
                  std::int64_t otherX, std::int64_t otherZ, std::uint64_t seed) {
    if (!adjacent(x,z,otherX,otherZ)) return false;
    return parent(x,z,seed) == ChunkCoord{otherX,otherZ}
        || parent(otherX,otherZ,seed) == ChunkCoord{x,z};
}

namespace {
bool hasDoor(std::int64_t x,std::int64_t z,std::int64_t bx,std::int64_t bz,std::uint64_t seed) {
    if (edgeReserved(x,z,bx,bz,seed) || sharedHall(x,z,bx,bz,seed)) return false;
    if (roomKind(x,z,seed)==RoomKind::DeadEnd || roomKind(bx,bz,seed)==RoomKind::DeadEnd) return false;
    if ((x==EXIT_CELL_X && z==EXIT_CELL_Z) || (bx==EXIT_CELL_X && bz==EXIT_CELL_Z)) return false;
    const double midX=(x+bx+1)*CELL*.5,midZ=(z+bz+1)*CELL*.5;
    // Nearby optional branch doors make the E interaction discoverable. None
    // can close a guaranteed route, and distant doors remain seed-driven.
    return std::hypot(midX-CELL/2,midZ-CELL/2)<CELL*2.34 || edgeHash(x,z,bx,bz,seed)%100<18;
}
}

bool edgeOpen(std::int64_t x, std::int64_t z,
              std::int64_t otherX, std::int64_t otherZ,
              std::uint64_t seed, std::uint64_t variant) {
    if (!adjacent(x,z,otherX,otherZ)) return false;
    if (edgeReserved(x,z,otherX,otherZ,seed)) return true;
    if (sharedHall(x,z,otherX,otherZ,seed)) return true;
    if (hasDoor(x,z,otherX,otherZ,seed)) return true;
    if (roomKind(x,z,seed)==RoomKind::DeadEnd || roomKind(otherX,otherZ,seed)==RoomKind::DeadEnd) return false;
    const bool sameChunk=floorDiv(x,CHUNK_CELLS)==floorDiv(otherX,CHUNK_CELLS)
                      && floorDiv(z,CHUNK_CELLS)==floorDiv(otherZ,CHUNK_CELLS);
    const auto salt=(sameChunk && variant != 0) ? mix(variant) : 0;
    return edgeHash(x,z,otherX,otherZ,seed ^ salt)%100 < 27;
}

World::World(std::uint64_t seed) : seed_(seed) {}

Chunk World::generate(ChunkCoord coord, std::uint64_t variant) const {
    Chunk chunk;
    chunk.coord=coord;
    chunk.maxHeight=static_cast<float>(ROOM_HEIGHT);
    chunk.vertices.reserve(14000);
    chunk.obstacles.reserve(100);
    chunk.overheads.reserve(200);
    constexpr float ceiling=static_cast<float>(ROOM_HEIGHT);
    for (int z=0; z<CHUNK_CELLS; ++z) {
        for (int x=0; x<CHUNK_CELLS; ++x) {
            const std::int64_t gx=coord.x*CHUNK_CELLS+x,gz=coord.z*CHUNK_CELLS+z;
            const float x0=static_cast<float>(x*CELL),z0=static_cast<float>(z*CELL);
            const float x1=x0+static_cast<float>(CELL),z1=z0+static_cast<float>(CELL);
            const auto room=roomInfo(gx,gz,seed_);
            const auto h=hashCell(gx,gz,seed_);
            // A room is wholly contained in a chunk: emit its floor and ceiling
            // once instead of duplicating flat surfaces for every cell.
            if (gx==room.x && gz==room.z) {
                const float width=static_cast<float>(room.widthCells*CELL);
                const float depth=static_cast<float>(room.depthCells*CELL);
                const auto surface=floorSurfaceAt(gx*CELL+CELL/2,gz*CELL+CELL/2,seed_);
                const float floorMaterial=surface==FloorSurface::Tile?19.0f:
                                          surface==FloorSurface::DampCarpet?18.0f:1.0f;
                quad(chunk,{x0,0,z0+depth},{x0+width,0,z0+depth},{x0+width,0,z0},{x0,0,z0},
                     {0,1,0},width,depth,floorMaterial);
                quad(chunk,{x0,ceiling,z0},{x0+width,ceiling,z0},
                     {x0+width,ceiling,z0+depth},{x0,ceiling,z0+depth},
                     {0,-1,0},width,depth,2);
                chunk.overheads.push_back({{x0,z0,x0+width,z0+depth},ceiling});
            }
            const bool eastOpen=edgeOpen(gx,gz,gx+1,gz,seed_,variant);
            const bool southOpen=edgeOpen(gx,gz,gx,gz+1,seed_,variant);
            const bool nearOrigin=std::abs(gx)<=1 && std::abs(gz)<=1;
            const bool eastHall=sharedHall(gx,gz,gx+1,gz,seed_);
            const bool southHall=sharedHall(gx,gz,gx,gz+1,seed_);
            const bool eastDoor=hasDoor(gx,gz,gx+1,gz,seed_);
            const bool southDoor=hasDoor(gx,gz,gx,gz+1,seed_);
            const bool eastWide=!nearOrigin && !eastDoor && edgeHash(gx,gz,gx+1,gz,seed_)%3==0;
            const bool southWide=!nearOrigin && !southDoor && edgeHash(gx,gz,gx,gz+1,seed_)%3==0;
            // Internal cell edges vanish, keeping the yellow rooms open.
            if (!eastHall) wall(chunk,true,x1,z0,eastOpen,eastWide);
            if (!southHall) wall(chunk,false,z1,x0,southOpen,southWide);
            auto addDoor=[&](bool east) {
                const auto id=edgeHash(gx,gz,gx+(east?1:0),gz+(east?0:1),seed_^0xd006ULL);
                const auto state=openDoors_.find(id);
                const bool open=state!=openDoors_.end() && state->second.target;
                const float openness=state==openDoors_.end()?0:state->second.openness;
                const float dx=east?x1:x0+static_cast<float>(CELL/2);
                const float dz=east?z0+static_cast<float>(CELL/2):z1;
                doorFrame(chunk,east?dx:dz,east?dz:dx,east,static_cast<float>(DOOR_HALF));
                chunk.doors.push_back({id,coord.x*CHUNK_SIZE+dx,coord.z*CHUNK_SIZE+dz,east,open,openness,false});
            };
            if (eastDoor) addDoor(true);
            if (southDoor) addDoor(false);
            // Columns sit on internal room corners, several metres away from
            // every reserved cell-centre route and all doorway openings.
            if (room.kind==RoomKind::PillarHall && gx+1<room.x+room.widthCells && gz+1<room.z+room.depthCells) {
                box(chunk,x1-.38f,0,z1-.38f,x1+.38f,ceiling,z1+.38f,0,true);
                box(chunk,x1-.405f,0,z1-.405f,x1+.405f,.14f,z1+.405f,3,false);
            }
            const float power=1.12f+static_cast<float>((h>>12)%16)*.01f;
            const auto stateRoll=(h>>20)%10;
            auto state=stateRoll<2?LampState::Off:stateRoll<5?LampState::Dying:LampState::Working;
            if (gx==0 && gz==0) state=LampState::Working;
            const float phase=(static_cast<float>((h>>40)&255)+.5f)/256;
            ceilingLight(chunk,x0+static_cast<float>(CELL/2),z0+static_cast<float>(CELL/2),power,state,phase);
            if (gx==EXIT_CELL_X && gz==EXIT_CELL_Z) {
                exitDoor(chunk,x0,z0);
                const auto id=hashCell(EXIT_CELL_X,EXIT_CELL_Z,seed_^0xe817d006ULL);
                const auto doorState=openDoors_.find(id);
                const bool open=doorState!=openDoors_.end() && doorState->second.target;
                const float openness=doorState==openDoors_.end()?0:doorState->second.openness;
                chunk.doors.push_back({id,(EXIT_CELL_X+1)*CELL-1.275,EXIT_CELL_Z*CELL+3.0,false,open,openness,true});
            }

            if (!eastHall && (!eastOpen || !eastWide) && (h>>24)%11==0)
                clue(chunk,x1-0.102f,z0+0.94f,true,-1);
            if (!southHall && (!southOpen || !southWide) && (h>>32)%13==0)
                clue(chunk,x0+0.94f,z1-0.102f,false,-1);
            // Facing north from the entrance, the clue is on the first left jamb.
            if (gx==0 && gz==-1)
                clue(chunk,x0+0.94f,z1+0.102f,false,1);
        }
    }
    return chunk;
}

void World::update(double x, double z, int radius) {
    // Cap caller mistakes to a reasonable working set; normal play uses radius 2.
    radius=std::clamp(radius,0,8);
    const auto center=chunkAt(x,z);
    if (radius_ == radius && center_ == center) return;
    const bool firstLoad=radius_<0;
    center_=center;
    radius_=radius;
    // Keep only the one-chunk outer ring. At the normal radius this is at
    // most 24 prepared chunks in addition to 25 playable resident chunks.
    for (auto it=prepared_.begin(); it!=prepared_.end();) {
        if (std::abs(it->first.x-center.x)>radius+1 || std::abs(it->first.z-center.z)>radius+1)
            it=prepared_.erase(it);
        else ++it;
    }
    for (auto it=chunks_.begin(); it!=chunks_.end();) {
        if (std::abs(it->first.x-center.x)>radius || std::abs(it->first.z-center.z)>radius) {
            const auto coord=it->first;
            // Unshifted geometry can serve a quick turn back. Shifted chunks
            // deliberately return to their seed layout once no longer resident.
            if (variants_.at(coord)==0 && std::abs(coord.x-center.x)<=radius+1 &&
                std::abs(coord.z-center.z)<=radius+1)
                prepared_.insert_or_assign(coord,std::move(it->second));
            variants_.erase(it->first);
            it=chunks_.erase(it);
        } else ++it;
    }
    for (std::int64_t cz=center.z-radius; cz<=center.z+radius; ++cz) {
        for (std::int64_t cx=center.x-radius; cx<=center.x+radius; ++cx) {
            const ChunkCoord coord{cx,cz};
            if (chunks_.find(coord)!=chunks_.end()) continue;
            Chunk chunk;
            const auto cached=prepared_.find(coord);
            if (cached!=prepared_.end()) {
                chunk=std::move(cached->second);
                prepared_.erase(cached);
                ++streamPromotions_;
            } else {
                chunk=generate(coord,0);
                chunk.revision=nextRevision_++;
                if (!firstLoad) ++streamFallbacks_;
            }
            chunks_.emplace(coord,std::move(chunk));
            variants_.emplace(coord,0);
        }
    }
}

bool World::prepareAhead(double x, double z) {
    if (radius_<0) return false;
    ChunkCoord candidate{};
    double best=std::numeric_limits<double>::infinity();
    const int outer=radius_+1;
    for (std::int64_t cz=center_.z-outer; cz<=center_.z+outer; ++cz) {
        for (std::int64_t cx=center_.x-outer; cx<=center_.x+outer; ++cx) {
            const ChunkCoord coord{cx,cz};
            if (chunks_.count(coord) || prepared_.count(coord)) continue;
            const double dx=(cx+.5)*CHUNK_SIZE-x,dz=(cz+.5)*CHUNK_SIZE-z;
            const double distance=dx*dx+dz*dz;
            if (distance<best) { best=distance; candidate=coord; }
        }
    }
    if (!std::isfinite(best)) return false;
    auto chunk=generate(candidate,0);
    chunk.revision=nextRevision_++;
    prepared_.emplace(candidate,std::move(chunk));
    return true;
}

bool World::blocked(double x, double z, double playerRadius,std::optional<std::uint64_t> ignoredDoorId) const {
    if (!std::isfinite(x) || !std::isfinite(z) || !std::isfinite(playerRadius)
        || playerRadius <= 0 || playerRadius > CELL/2) return true;
    const auto low=chunkAt(x-playerRadius,z-playerRadius);
    const auto high=chunkAt(x+playerRadius,z+playerRadius);
    // Do not enter non-resident floor space, including any part of the capsule.
    for (auto cz=low.z; cz<=high.z; ++cz)
        for (auto cx=low.x; cx<=high.x; ++cx)
            if (chunks_.find({cx,cz})==chunks_.end()) return true;

    // A western/northern neighbour owns walls that protrude into this chunk.
    // Expand by wall thickness, even when the player's circle doesn't cross it.
    const auto obstacleLow=chunkAt(x-playerRadius-WALL_HALF-0.02,z-playerRadius-WALL_HALF-0.02);
    const auto obstacleHigh=chunkAt(x+playerRadius+WALL_HALF+0.02,z+playerRadius+WALL_HALF+0.02);
    for (auto cz=obstacleLow.z; cz<=obstacleHigh.z; ++cz) {
        for (auto cx=obstacleLow.x; cx<=obstacleHigh.x; ++cx) {
            const auto found=chunks_.find({cx,cz});
            if (found==chunks_.end()) continue;
            const double localX=x-cx*CHUNK_SIZE,localZ=z-cz*CHUNK_SIZE;
            for (const auto& obstacle:found->second.obstacles)
                if (circleHits(obstacle,localX,localZ,playerRadius)) return true;
        }
    }
    // A leaf can extend into either neighbouring chunk. Its maximum swing is
    // shorter than2.3m, so only nearby owners can possibly contain a hit. The
    // extra margin includes leaves whose centre lies on their owner's border.
    const double doorReach=2*DOOR_HALF+DOOR_THICKNESS+.02+playerRadius;
    const auto doorLow=chunkAt(x-doorReach,z-doorReach),doorHigh=chunkAt(x+doorReach,z+doorReach);
    for (auto cz=doorLow.z;cz<=doorHigh.z;++cz) for (auto cx=doorLow.x;cx<=doorHigh.x;++cx) {
        const auto found=chunks_.find({cx,cz});
        if (found==chunks_.end()) continue;
        for (const auto& door:found->second.doors)
            if ((!ignoredDoorId || door.id!=*ignoredDoorId) && doorHits(door,door.openness,x,z,playerRadius)) return true;
    }
    return false;
}

float World::headClearance(double x,double z,double radius) const {
    if (!std::isfinite(x) || !std::isfinite(z) || !std::isfinite(radius)
        || radius<=0 || radius>CELL/2) return 0;
    const auto low=chunkAt(x-radius,z-radius),high=chunkAt(x+radius,z+radius);
    for (auto cz=low.z;cz<=high.z;++cz) for (auto cx=low.x;cx<=high.x;++cx)
        if (chunks_.find({cx,cz})==chunks_.end()) return 0;
    // Header/fixture geometry can protrude a little beyond its owner's edge.
    // The widest such trim is0.125m, including a door on a negative seam.
    const auto ownersLow=chunkAt(x-radius-.15,z-radius-.15);
    const auto ownersHigh=chunkAt(x+radius+.15,z+radius+.15);
    float clearance=static_cast<float>(ROOM_HEIGHT);
    for (auto cz=ownersLow.z;cz<=ownersHigh.z;++cz) for (auto cx=ownersLow.x;cx<=ownersHigh.x;++cx) {
        const auto found=chunks_.find({cx,cz}); if (found==chunks_.end()) continue;
        const double localX=x-cx*CHUNK_SIZE,localZ=z-cz*CHUNK_SIZE;
        for (const auto& overhead:found->second.overheads)
            if (overhead.undersideY<clearance && circleHits(overhead.footprint,localX,localZ,radius))
                clearance=overhead.undersideY;
    }
    return clearance;
}

bool World::nearestDoor(double x,double z,double range,DoorInfo& result) const {
    if (!std::isfinite(x) || !std::isfinite(z) || !std::isfinite(range) || range<=0) return false;
    bool found=false;
    double closest=range*range;
    for (const auto& [coord,chunk]:chunks_) {
        (void)coord;
        for (const auto& door:chunk.doors) {
            const double dx=door.x-x,dz=door.z-z,distance=dx*dx+dz*dz;
            if (distance<=closest) { result=door; closest=distance; found=true; }
        }
    }
    return found;
}

bool World::toggleDoor(std::uint64_t id,double playerX,double playerZ,double playerRadius) {
    if (!std::isfinite(playerX) || !std::isfinite(playerZ) || !std::isfinite(playerRadius) ||
        playerRadius<=0 || playerRadius>CELL/2) return false;
    ChunkCoord owner{};
    DoorInfo selected{};
    bool found=false;
    for (const auto& [coord,chunk]:chunks_) {
        for (const auto& door:chunk.doors) if (door.id==id) {
            owner=coord; selected=door; found=true; break;
        }
        if (found) break;
    }
    if (!found || std::hypot(playerX-selected.x,playerZ-selected.z)>3.0) return false;
    if (selected.open && doorHits(selected,0,playerX,playerZ,playerRadius)) return false;
    openDoors_[id]={owner,++doorOrder_,selected.openness,!selected.open};
    syncDoor(id);
    if (openDoors_.size()>128) {
        auto oldest=openDoors_.end();
        for (auto it=openDoors_.begin();it!=openDoors_.end();++it) {
            if (it->first==id) continue;
            // Never forget a leaf next to the player while restoring its seeded
            // state. All other forgotten leaves are outside collision reach.
            bool near=false;
            const auto active=chunks_.find(it->second.owner);
            if (active!=chunks_.end())
                for (const auto& door:active->second.doors)
                    if (door.id==it->first && std::hypot(door.x-playerX,door.z-playerZ)<4) near=true;
            if (!near && (oldest==openDoors_.end() || it->second.order<oldest->second.order)) oldest=it;
        }
        if (oldest!=openDoors_.end()) {
            const auto expired=oldest->first;
            openDoors_.erase(oldest);
            syncDoor(expired);
        }
    }
    return true;
}

void World::syncDoor(std::uint64_t id) {
    const auto state=openDoors_.find(id);
    auto sync=[&](auto& chunks) {
        for (auto& [coord,chunk]:chunks) {
            (void)coord;
            for (auto& door:chunk.doors) if (door.id==id) {
                door.open=state!=openDoors_.end() && state->second.target;
                door.openness=state==openDoors_.end()?0:state->second.openness;
            }
        }
    };
    sync(chunks_);
    sync(prepared_);
}

bool World::advanceDoors(float dt,double playerX,double playerZ,double playerRadius,bool paused,
                         std::optional<WorldPoint> otherActor,double otherRadius) {
    if (paused || !std::isfinite(dt) || dt<=0 || !std::isfinite(playerX) || !std::isfinite(playerZ)
        || !std::isfinite(playerRadius) || playerRadius<=0 || playerRadius>CELL/2) return false;
    bool changed=false;
    for (auto it=openDoors_.begin();it!=openDoors_.end();) {
        auto& state=it->second;
        const auto chunk=chunks_.find(state.owner);
        if (chunk==chunks_.end()) { ++it; continue; }
        const auto selected=std::find_if(chunk->second.doors.begin(),chunk->second.doors.end(),
            [&](const DoorInfo& door) { return door.id==it->first; });
        if (selected==chunk->second.doors.end()) { ++it; continue; }
        const float target=state.target?1.0f:0.0f;
        const float before=state.openness;
        const float movement=std::min(std::abs(target-before),std::min(dt,1.0f)*1.25f);
        // Substeps prevent a long frame sweeping the panel through the player.
        const int steps=std::max(1,static_cast<int>(std::ceil(movement/.01f)));
        for (int step=0;step<steps && state.openness!=target;++step) {
            const float next=state.target?std::min(target,state.openness+movement/steps)
                                         :std::max(target,state.openness-movement/steps);
            const bool touchesActor=otherActor && std::isfinite(otherActor->x) && std::isfinite(otherActor->z)
                && otherRadius>0 && otherRadius<CELL/2
                && doorHits(*selected,next,otherActor->x,otherActor->z,otherRadius);
            if (doorHits(*selected,next,playerX,playerZ,playerRadius) || touchesActor) {
                if (!state.target) state.target=true; // A closing door yields.
                break;
            }
            state.openness=next;
        }
        if (std::abs(state.openness-target)<.00001f) state.openness=target;
        changed=changed || state.openness!=before;
        const auto id=it->first;
        if (!state.target && state.openness==0) it=openDoors_.erase(it);
        else ++it;
        syncDoor(id);
    }
    return changed;
}

std::vector<Vertex> World::doorVertices(double originX,double originZ) const {
    Chunk panels{};
    for (const auto& [coord,chunk]:chunks_) {
        (void)coord;
        for (const auto& door:chunk.doors) {
            const auto pose=doorPose(door,door.openness);
            const auto first=panels.vertices.size();
            box(panels,0,.015f,-static_cast<float>(DOOR_THICKNESS),static_cast<float>(pose.width),2.40f,
                static_cast<float>(DOOR_THICKNESS),17);
            box(panels,static_cast<float>(pose.width)-.26f,.99f,-.082f,
                static_cast<float>(pose.width)-.20f,1.22f,.082f,7);
            for (std::size_t i=first;i<panels.vertices.size();++i) {
                auto& v=panels.vertices[i];
                const float x=v.x,z=v.z,nx=v.nx,nz=v.nz;
                v.x=static_cast<float>(pose.hingeX-originX+x*pose.alongX-z*pose.alongZ);
                v.z=static_cast<float>(pose.hingeZ-originZ+x*pose.alongZ+z*pose.alongX);
                v.nx=static_cast<float>(nx*pose.alongX-nz*pose.alongZ);
                v.nz=static_cast<float>(nx*pose.alongZ+nz*pose.alongX);
            }
        }
    }
    return std::move(panels.vertices);
}

bool World::exitCrossed(double playerX,double playerZ) const {
    const auto approach=exitLocation();
    const double front=approach.z-1.2;
    if (!std::isfinite(playerX) || !std::isfinite(playerZ) || std::abs(playerX-approach.x)>EXIT_HALF-.3
        || playerZ>=front-.45 || playerZ<=front-2.65) return false;
    DoorInfo exit{};
    return nearestDoor(approach.x,front,.1,exit) && exit.isExit && exit.openness>=.98f;
}

bool World::shiftBehind(double x, double z, float forwardX, float forwardZ,std::optional<WorldPoint> protectedActor) {
    const double length=std::hypot(forwardX,forwardZ);
    if (length<0.01) return false;
    const double fx=forwardX/length,fz=forwardZ/length;
    // Include the complete square and its wall thickness in the visibility test.
    constexpr double bound=CHUNK_SIZE*0.7071067811865475244+0.25;
    auto chosen=chunks_.end();
    std::optional<ChunkCoord> protectedChunk;
    if (protectedActor && std::isfinite(protectedActor->x) && std::isfinite(protectedActor->z))
        protectedChunk=chunkAt(protectedActor->x,protectedActor->z);
    double bestScore=-std::numeric_limits<double>::infinity();
    for (auto it=chunks_.begin(); it!=chunks_.end(); ++it) {
        if (protectedChunk && std::abs(it->first.x-protectedChunk->x)<=1
            && std::abs(it->first.z-protectedChunk->z)<=1) continue;
        const double dx=(it->first.x+0.5)*CHUNK_SIZE-x;
        const double dz=(it->first.z+0.5)*CHUNK_SIZE-z;
        const double distance=std::hypot(dx,dz), forwardDistance=dx*fx+dz*fz;
        if (distance-bound <= 45 || forwardDistance+bound >= -6) continue;
        const double score=-forwardDistance+static_cast<double>(hashCell(it->first.x,it->first.z,seed_+shifts_)%31);
        if (score > bestScore) { bestScore=score; chosen=it; }
    }
    if (chosen==chunks_.end()) return false;
    const auto coord=chosen->first;
    const auto variant=++variants_.at(coord);
    auto replacement=generate(coord,variant);
    replacement.revision=nextRevision_++;
    chosen->second=std::move(replacement);
    ++shifts_;
    return true;
}

} // namespace br
