#pragma once
#include "gl_resource.h"

namespace br {

// Original, deterministic room RGB textures generated on the CPU at startup.
// Wallpaper uses a 12 m sheet with baked wear and origin-stable world mapping.
// Carpet, ceiling and hard floor repeat every metre through mesh UVs. Normal
// maps encode tangent-space XYZ in linear RGB, generated from physical heights.
struct Textures {
    gl::Texture wallpaper;
    gl::Texture carpet;
    gl::Texture ceiling;
    gl::Texture wallpaperNormal;
    gl::Texture carpetNormal;
    gl::Texture ceilingNormal;
    gl::Texture hardFloor;
    gl::Texture hardFloorNormal;
    // Supplied Aranea source images converted offline into bounded RGB caches.
    // Color is sRGB (decoded in the shader); normal values remain linear.
    gl::Texture creatureColor;
    gl::Texture creatureNormal;
};

Textures createTextures();
void destroyTextures(Textures& textures);

} // namespace br
