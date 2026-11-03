#pragma once

#include "../common/math.h"

namespace br {
class UI;
class World;
namespace app {

void minimap(UI& ui, const World& world, double px, double pz,
             Vec3 forward, int width, bool clues);

} // namespace app
} // namespace br
