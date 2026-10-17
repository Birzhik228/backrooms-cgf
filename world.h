#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <vector>

namespace br {

inline constexpr double CELL = 7.5;
inline constexpr double ROOM_HEIGHT = 3.2;
inline constexpr int CHUNK_CELLS = 6;
inline constexpr double CHUNK_SIZE = CELL * CHUNK_CELLS;
struct Vertex { float x,y,z,nx,ny,nz,u,v,material; };
struct ChunkCoord {
    std::int64_t x, z;
    bool operator<(const ChunkCoord& other) const {
        return x < other.x || (x == other.x && z < other.z);
    }
    bool operator==(const ChunkCoord& other) const { return x == other.x && z == other.z; }
};
ChunkCoord exitCell(std::uint64_t seed);
struct Aabb { double minX,minZ,maxX,maxZ; };
// Local chunk-space footprint of elevated solid geometry. Full-height walls
// stay in obstacles; these records measure room available above a tall actor.
struct Overhead { Aabb footprint; float undersideY; };
struct SightBlocker { Aabb footprint; float bottom,top; };
enum class LampState { Working=0, Dying=1, Off=2 };
struct Lamp {
    float x,y,z,power;
    float r=1.0f,g=0.965f,b=0.77f;
    LampState state=LampState::Working;
    float phase=0;
};
struct WorldPoint { double x,z; };
struct HidingSpot {
    std::uint64_t id;
    WorldPoint inside, approach;
};
enum class RoomKind { Classic, Corridor, OpenHall, PillarHall, DeadEnd, OddRoom };
enum class FloorSurface { Carpet, DampCarpet, Tile };
enum class Level { Zero, Fun };
struct DoorInfo {
    std::uint64_t id;
    double x,z; // Global midpoint, also used for interaction and spatial audio.
    bool east,open; // East means the panel lies on a plane of constant X.
    float openness=0; // Hinge animation progress; open is the requested target.
    bool isExit=false;
    float swing=1; // Signed hinge direction, chosen away from the opener.
};
struct RoomInfo {
    std::int64_t x,z; // Global cell coordinates of this room's northwest corner.
    int widthCells,depthCells;
    float height;
    RoomKind kind;
};
struct Chunk {
    ChunkCoord coord;
    std::uint64_t revision = 0;
    float maxHeight = 0;
    std::vector<Vertex> vertices;
    // Optional render-only distant mesh. Collision always uses the full world.
    std::vector<Vertex> distantVertices;
    std::vector<Aabb> obstacles;
    std::vector<Overhead> overheads;
    std::vector<SightBlocker> sightBlockers;
    std::vector<Lamp> lamps;
    std::vector<DoorInfo> doors;
    std::vector<HidingSpot> hidingSpots;
};

std::int64_t floorDiv(std::int64_t value, std::int64_t divisor);
ChunkCoord chunkAt(double x, double z);
std::uint64_t hashCell(std::int64_t x, std::int64_t z, std::uint64_t seed);
RoomKind roomKind(std::int64_t x, std::int64_t z, std::uint64_t seed);
RoomInfo roomInfo(std::int64_t x, std::int64_t z, std::uint64_t seed);
const char* roomName(RoomKind kind);
FloorSurface floorSurfaceAt(double x,double z,std::uint64_t seed);
float lampEnvelope(LampState state,float phase,float time);

// Each cell has a permanently open parent edge toward the origin. Additional
// openings vary by seed; the decision is symmetric for either edge endpoint.
bool edgeReserved(std::int64_t x, std::int64_t z,
                  std::int64_t otherX, std::int64_t otherZ, std::uint64_t seed);
bool edgeOpen(std::int64_t x, std::int64_t z,
              std::int64_t otherX, std::int64_t otherZ,
              std::uint64_t seed, std::uint64_t variant = 0);

class World {
public:
    explicit World(std::uint64_t seed,Level level=Level::Zero);
    Level level() const { return level_; }
    bool isFun() const { return level_==Level::Fun; }
    void update(double x, double z, int radius = 2);
    const std::map<ChunkCoord,Chunk>& chunks() const { return chunks_; }
    // Optional game-loop prefetch. Synchronous update() remains complete for
    // tests/teleports; ordinary travel promotes these already prepared meshes.
    bool prepareAhead(double x, double z);
    const std::map<ChunkCoord,Chunk>& preparedChunks() const { return prepared_; }
    std::size_t streamPromotions() const { return streamPromotions_; }
    std::size_t streamFallbacks() const { return streamFallbacks_; }
    // The optional exclusion is for interaction visibility only: a selected
    // leaf must not hide its own handle. Movement supplies no exclusion.
    bool blocked(double x, double z, double playerRadius,
                 std::optional<std::uint64_t> ignoredDoorId=std::nullopt) const;
    // Minimum actual underside over a circular footprint; radius1 anticipates
    // a tall head approaching a lintel. Invalid/nonresident queries return0.
    float headClearance(double x,double z,double radius=1.0) const;
    // A live actor and its neighbouring chunks must keep their collision mesh
    // stable, even when they are distant and behind the player's camera.
    bool shiftBehind(double x, double z, float forwardX, float forwardZ,
                     std::optional<WorldPoint> protectedActor=std::nullopt,
                     const std::vector<WorldPoint>& protectedActors={});
    bool nearestDoor(double x,double z,double range,DoorInfo& result) const;
    bool toggleDoor(std::uint64_t id,double playerX,double playerZ,double playerRadius=.25);
    bool advanceDoors(float dt,double playerX,double playerZ,double playerRadius=.25,bool paused=false,
                      std::optional<WorldPoint> otherActor=std::nullopt,double otherRadius=.3);
    std::vector<Vertex> doorVertices(double originX,double originZ) const;
    bool exitCrossed(double playerX,double playerZ) const;
    WorldPoint keyLocation() const;
    bool keyCollected() const { return keyCollected_; }
    bool collectKey(double x,double z);
    bool nearestHidingSpot(double x,double z,double range,HidingSpot& result) const;
    std::vector<Vertex> keyVertices(double originX,double originZ) const;
    float soundTransmission(double ax,double az,double bx,double bz) const;
    // The latest 128 opened doors survive unloading. Older overrides return to
    // their seeded closed state, keeping memory bounded for endless travel.
    std::size_t rememberedDoors() const { return openDoors_.size(); }
    std::uint64_t seed() const { return seed_; }
    std::size_t shifts() const { return shifts_; }
    WorldPoint exitLocation() const { const auto c=exitCell(seed_); return {(c.x+1)*CELL-1.275,c.z*CELL+4.2}; }
private:
    Chunk generate(ChunkCoord coord, std::uint64_t variant) const;
    std::uint64_t seed_;
    Level level_=Level::Zero;
    bool keyCollected_=false;
    std::size_t shifts_ = 0;
    std::uint64_t nextRevision_ = 1;
    std::map<ChunkCoord,Chunk> chunks_;
    std::map<ChunkCoord,std::uint64_t> variants_;
    std::map<ChunkCoord,Chunk> prepared_;
    struct OpenDoor { ChunkCoord owner; std::uint64_t order; float openness; bool target; float swing=1; };
    std::map<std::uint64_t,OpenDoor> openDoors_;
    std::uint64_t doorOrder_=0;
    ChunkCoord center_{0,0};
    int radius_ = -1;
    std::size_t streamPromotions_ = 0, streamFallbacks_ = 0;
    void syncDoor(std::uint64_t id);
};

} // namespace br
