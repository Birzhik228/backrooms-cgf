#include "minimap.h"
#include "../common/ui.h"
#include "../common/parking_objective.h"
#include "../world.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace br::app {

void minimap(UI& ui, const World& world, double px, double pz,
                    Vec3 forward, int width, bool clues) {
    constexpr float size = 228, scale = 2.3f;
    const float left = float(width) - size - 26, top = 80;
    const float cx = left + size * .5f, cy = top + size * .5f;
    ui.rect(left, top, size, size, {.035f, .04f, .032f, .91f});
    ui.rect(left, top, size, 2, {.7f, .65f, .36f, .7f});

    for (const auto& [coord, chunk] : world.chunks()) {
        const double ox = coord.x * CHUNK_SIZE, oz = coord.z * CHUNK_SIZE;
        for (const auto& bounds : chunk.obstacles) {
            float x = cx + float(ox + bounds.minX - px) * scale;
            float y = cy + float(oz + bounds.minZ - pz) * scale;
            float x2 = cx + float(ox + bounds.maxX - px) * scale;
            float y2 = cy + float(oz + bounds.maxZ - pz) * scale;
            x = std::clamp(x, left + 5, left + size - 5);
            x2 = std::clamp(x2, left + 5, left + size - 5);
            y = std::clamp(y, top + 5, top + size - 5);
            y2 = std::clamp(y2, top + 5, top + size - 5);
            if (x2 > x && y2 > y) {
                ui.rect(x, y, std::max(1.f, x2 - x), std::max(1.f, y2 - y),
                        {.51f, .49f, .33f, .8f});
            }
        }
    }
    {
    const auto door = world.exitLocation();
    const float exitX = cx + float(door.x - px) * scale;
    const float exitY = cy + float(door.z - pz) * scale;
    if (exitX > left + 8 && exitX < left + size - 8 &&
        exitY > top + 8 && exitY < top + size - 8) {
        ui.rect(exitX - 4, exitY - 4, 8, 8, {.3f, 1.f, .65f, 1});
        ui.text(std::min(exitX + 7, left + size - 36), exitY - 3, "EXIT", 1, {.5f, 1, .75f, 1});
    }
    if(world.isParking())for(int i=0;i<3;++i) {
        const auto p=world.breakerLocation(i);
        const float bx=std::clamp(cx+float(p.x-px)*scale,left+10,left+size-20);
        const float bz=std::clamp(cy+float(p.z-pz)*scale,top+10,top+size-12);
        if(i==parkingCircuitTarget(world,px,pz)) {
            ui.rect(bx-4,bz-4,17,18,{.92f,.89f,.70f,1});
            ui.rect(bx-3,bz-3,15,16,{.045f,.055f,.04f,1});
        }
        ui.text(bx,bz,std::string(1,char('A'+i)),1.4f,world.breakerPowered(i)?Color{.3f,1,.65f,1}:Color{1,.6f,.25f,1});
    }
    if(world.level()==Level::Zero && !world.keyCollected()) {
        const auto key=world.keyLocation();
        const float kx=std::clamp(cx+float(key.x-px)*scale,left+10,left+size-30);
        const float ky=std::clamp(cy+float(key.z-pz)*scale,top+10,top+size-12);
        ui.rect(kx-3,ky-3,6,6,{1,.78f,.25f,1});ui.text(kx+7,ky-3,"KEY",1,{1,.78f,.25f,1});
    }
    }
    for(const auto& [coord,chunk]:world.chunks()) for(const auto& spot:chunk.hidingSpots) {
        (void)coord;
        const float sx=cx+float(spot.approach.x-px)*scale,sy=cy+float(spot.approach.z-pz)*scale;
        if(sx>left+8 && sx<left+size-15 && sy>top+8 && sy<top+size-12) ui.text(sx,sy,"H",1,{.65f,.72f,.65f,1});
    }
    if(world.isFun() && clues && !world.keyCollected()) {
        for(const auto& entry:world.chunks())for(const auto& gift:entry.second.presents) {
            if(gift.opened || std::hypot(gift.x-px,gift.z-pz)>32)continue;
            const float gx=cx+float(gift.x-px)*scale,gz=cy+float(gift.z-pz)*scale;
            ui.rect(gx-3,gz-3,6,6,{.94f,.56f,.72f,1});
            ui.line(gx,gz-3,gx,gz+3,1,{1,.9f,.78f,1});
        }
        ui.text(left+10,top+size+27,"PINK / UNOPENED PRESENTS",1.1f,{.94f,.56f,.72f,1});
    }
    ui.rect(cx - 3, cy - 3, 6, 6, {.95f, .8f, .37f, 1});
    ui.line(cx, cy, cx + forward.x * 15, cy + forward.z * 15, 2, {1, .9f, .6f, 1});
    ui.text(left + 10, top + size + 10, "LOCAL MAP / LOADED AREA",
            1.25f, {.8f, .78f, .62f, 1});

}

} // namespace br::app
