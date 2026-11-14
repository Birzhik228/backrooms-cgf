#pragma once
#include "world.h"
#include <map>

namespace br::app {

// Opt-in runtime verification snapshots. The ordinary game never reads these.
struct DemoControlsProbe {
    bool captured=false, verified=false, key=false;
    double playerX=0, playerZ=0, threatX=0, threatZ=0, elapsed=0;
    double cameraX=0, cameraZ=0, partyTravel=0;
    float cameraY=0, stamina=0, yaw=0, pitch=0, animation=0, bob=0, aoAmount=0;
    unsigned aoPasses=0;
    int circuits=0, presents=0, doors=0;
    std::map<ChunkCoord,std::uint64_t> chunkRevisions;
};

} // namespace br::app
