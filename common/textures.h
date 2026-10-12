#pragma once

namespace br {

// Original, deterministic room RGB textures generated on the CPU at startup.
// Wallpaper uses a 12 m sheet with baked wear and origin-stable world mapping.
// Carpet, ceiling and hard floor repeat every metre through mesh UVs. Normal
// maps encode tangent-space XYZ in linear RGB, generated from physical heights.
struct Textures {
    unsigned int wallpaper = 0;
    unsigned int carpet = 0;
    unsigned int ceiling = 0;
    unsigned int wallpaperNormal = 0;
    unsigned int carpetNormal = 0;
    unsigned int ceilingNormal = 0;
    unsigned int hardFloor = 0;
    unsigned int hardFloorNormal = 0;
    // Supplied Aranea source images converted offline into bounded RGB caches.
    // Color is sRGB (decoded in the shader); normal values remain linear.
    unsigned int creatureColor = 0;
    unsigned int creatureNormal = 0;
};

Textures createTextures();
void destroyTextures(Textures& textures);

} // namespace br
