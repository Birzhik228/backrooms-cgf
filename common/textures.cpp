#include "textures.h"

#include <glad/glad.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace br {
namespace {

constexpr int kSize = 512;
constexpr int kWallWidth = 1536;
constexpr int kWallHeight = 512;
constexpr float kWallMeters = 12.0f;
constexpr float kWallHeightMeters = 3.2f;
constexpr float kPi = 3.14159265358979323846f;

float clamp01(float x) { return std::max(0.0f, std::min(1.0f, x)); }
float fract(float x) { return x - std::floor(x); }
float smooth(float a, float b, float x) {
    const float t = clamp01((x - a) / (b - a));
    return t * t * (3.0f - 2.0f * t);
}
float mix(float a, float b, float t) { return a + (b - a) * t; }

std::uint32_t hash(std::uint32_t value) {
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    return value ^ (value >> 16);
}

// The lattice wraps, so bilinear filtering does not expose a texture seam.
float lattice(int x, int y, int period, std::uint32_t seed) {
    x = (x % period + period) % period;
    y = (y % period + period) % period;
    return float(hash(std::uint32_t(x) + std::uint32_t(y) * 65537u + seed) & 0xffffu) / 65535.0f;
}

float noise(float u, float v, int period, std::uint32_t seed) {
    const float x = u * float(period), y = v * float(period);
    const int ix = int(std::floor(x)), iy = int(std::floor(y));
    const float fx = smooth(0.0f, 1.0f, fract(x));
    const float fy = smooth(0.0f, 1.0f, fract(y));
    return mix(mix(lattice(ix, iy, period, seed), lattice(ix + 1, iy, period, seed), fx),
               mix(lattice(ix, iy + 1, period, seed), lattice(ix + 1, iy + 1, period, seed), fx), fy);
}

float leaf(float x, float y, float cx, float cy, float direction) {
    const float dx = x - cx, dy = y - cy;
    const float a = dx * 0.77f + dy * direction * 0.64f;
    const float b = -dx * direction * 0.64f + dy * 0.77f;
    const float ellipse = a * a / (0.112f * 0.112f) + b * b / (0.034f * 0.034f);
    return 1.0f - smooth(0.55f, 1.25f, ellipse);
}

void pixel(std::vector<unsigned char>& image, int x, int y, float r, float g, float b,
           int width = kSize) {
    const int index = (y * width + x) * 3;
    image[index] = static_cast<unsigned char>(clamp01(r) * 255.0f + 0.5f);
    image[index + 1] = static_cast<unsigned char>(clamp01(g) * 255.0f + 0.5f);
    image[index + 2] = static_cast<unsigned char>(clamp01(b) * 255.0f + 0.5f);
}

unsigned int upload(const std::vector<unsigned char>& image, int width = kSize, int height = kSize) {
    unsigned int id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    return id;
}

// Central differences use the physical texture size, so a millimetre of
// wallpaper relief does not become a metre-scale bump on the larger sheet.
// Repeat-wrapped neighbours keep mipmapped normal maps continuous at seams.
unsigned int uploadNormals(const std::vector<float>& heights, int width, int height,
                           float metersWide, float metersHigh) {
    std::vector<unsigned char> normals(std::size_t(width) * height * 3);
    const float dxScale = float(width) / (2.0f * metersWide);
    const float dyScale = float(height) / (2.0f * metersHigh);
    for (int y = 0; y < height; ++y) {
        const int down = (y + height - 1) % height, up = (y + 1) % height;
        for (int x = 0; x < width; ++x) {
            const int left = (x + width - 1) % width, right = (x + 1) % width;
            const float nx = -(heights[y * width + right] - heights[y * width + left]) * dxScale;
            const float ny = -(heights[up * width + x] - heights[down * width + x]) * dyScale;
            const float inverseLength = 1.0f / std::sqrt(nx * nx + ny * ny + 1.0f);
            pixel(normals, x, y, nx * inverseLength * 0.5f + 0.5f,
                  ny * inverseLength * 0.5f + 0.5f, inverseLength * 0.5f + 0.5f, width);
        }
    }
    return upload(normals, width, height);
}

struct PaperScar { float x, y, width, height, lean; bool peeled; };

std::array<PaperScar, 11> paperScars() {
    std::array<PaperScar, 11> scars{};
    for (std::size_t i = 0; i < scars.size(); ++i) {
        const auto random = [i](std::uint32_t salt) {
            return float(hash(std::uint32_t(i) * 7919u + salt) & 0xffffu) / 65535.0f;
        };
        const bool peeled = i % 3 == 0;
        scars[i] = {
            (float(i) + 0.12f + random(121u) * 0.76f) * kWallMeters / float(scars.size()),
            0.63f + random(283u) * 1.91f,
            peeled ? 0.065f + random(349u) * 0.115f : 0.012f + random(349u) * 0.010f,
            0.19f + random(761u) * 0.43f,
            (random(1229u) - 0.5f) * 0.35f, peeled
        };
    }
    return scars;
}

// Damage is baked once into the same wallpaper texture. Its larger physical
// sheet avoids a conspicuous scratch stamp on every one-metre floral repeat.
// It adds no meshes or runtime noise evaluation. The same damage also shapes
// the height field used to bake the wallpaper's tangent-space normal map.
void scarColor(const std::array<PaperScar, 11>& scars, float x, float y, float roughness,
               float& r, float& g, float& b) {
    for (const auto& scar : scars) {
        float dx = x - scar.x;
        dx -= std::floor(dx / kWallMeters + 0.5f) * kWallMeters;
        const float dy = y - scar.y;
        if (std::abs(dy) > scar.height * 1.12f ||
            std::abs(dx) > scar.width * 2.2f + scar.height * std::abs(scar.lean) + 0.15f)
            continue;
        dx += dy * scar.lean;
        const float end = std::abs(dy) / scar.height;
        const float ragged = std::sin(dy * 47.0f + scar.x) * 0.09f
                           + std::sin(dy * 103.0f) * 0.04f;
        if (scar.peeled) {
            const float taper = 0.24f + 0.76f * std::sqrt(std::max(0.0f, 1.0f - end * end));
            const float edge = std::abs(dx) / (scar.width * taper) + ragged;
            const float outline = std::max(edge, end);
            const float torn = 1.0f - smooth(0.84f, 1.01f, outline);
            const float lip = (1.0f - smooth(1.01f, 1.16f, outline)) - torn;
            // Exposed brown backing and an uneven curled edge. The lighter
            // right-hand lip is baked pigment, not dynamic displaced geometry.
            const float backing = roughness * 0.035f;
            r = mix(r, 0.515f + backing, torn);
            g = mix(g, 0.444f + backing, torn);
            b = mix(b, 0.293f + backing, torn);
            r = mix(r, dx > 0 ? 0.84f : 0.32f, lip * 0.85f);
            g = mix(g, dx > 0 ? 0.77f : 0.27f, lip * 0.85f);
            b = mix(b, dx > 0 ? 0.53f : 0.16f, lip * 0.85f);
        } else {
            // Small clusters of uneven slashes, with separate start/end points.
            float scratches = 0.0f;
            for (int line = -1; line <= 1; ++line) {
                const float offset = float(line) * 0.056f;
                const float wobble = 0.004f * std::sin(dy * 41.0f + float(line) * 3.0f);
                const float width = scar.width * (0.74f + roughness * 0.52f);
                const float stroke = 1.0f - smooth(width * 0.28f, width,
                                                   std::abs(dx + offset + wobble));
                const float tip = 1.0f - smooth(0.60f, 0.99f,
                                                 end + float(line) * dy * 0.15f + ragged);
                scratches = std::max(scratches, stroke * tip);
            }
            r = mix(r, 0.34f, scratches * 0.84f);
            g = mix(g, 0.285f, scratches * 0.84f);
            b = mix(b, 0.173f, scratches * 0.84f);
        }
    }
}

} // namespace

Textures createTextures() {
    Textures textures;
    std::vector<unsigned char> image(kWallWidth * kWallHeight * 3);
    std::vector<float> heights(kWallWidth * kWallHeight);
    const auto scars = paperScars();

    for (int y = 0; y < kWallHeight; ++y) {
        for (int x = 0; x < kWallWidth; ++x) {
            const float u = float(x) / kWallWidth, v = float(y) / kWallHeight;
            const float metersX = u * kWallMeters, metersY = v * kWallHeightMeters;
            const float paper = (noise(u, v, 8, 17u) - 0.5f) * 0.027f
                              + (noise(u, v, 64, 71u) - 0.5f) * 0.018f;
            const float grain = (lattice(x, y, kSize, 111u) - 0.5f) * 0.014f;
            const float sx = fract(metersX * 4.0f) - 0.5f, sy = fract(metersY * 4.0f);
            const float curve = 0.028f * std::sin(sy * 2.0f * kPi);
            const float stem = 1.0f - smooth(0.009f, 0.025f, std::abs(sx - curve));
            const float leaves = leaf(sx, sy, 0.07f, 0.28f, 1.0f)
                               + leaf(sx, sy, -0.07f, 0.72f, -1.0f);
            const float fineStripe = 1.0f - smooth(0.006f, 0.018f, std::abs(fract(metersX * 8.0f) - 0.5f));
            const float ink = (stem * 0.60f + leaves * 0.80f + fineStripe * 0.16f) * 0.075f;
            // Dark opening seams between half-metre wallpaper strips, long
            // vertical water runs, and low scuffs survive the stronger lens.
            const float strip = fract(metersX * 2.0f);
            const float join = 1.0f - smooth(0.004f, 0.024f, std::min(strip, 1.0f-strip));
            const float wearNoise = noise(u, v, 16, 977u);
            const float damp = smooth(0.57f, 0.82f, noise(u, v * 0.25f, 32, 2719u));
            const float dampFade = 1.0f - smooth(1.55f, 3.10f, metersY);
            const float scuffHeight = smooth(0.12f, 0.35f, metersY)
                                    * (1.0f - smooth(0.62f, 1.02f, metersY));
            const float scuffs = smooth(0.57f, 0.78f, noise(u, v * 4.0f, 16, 787u))
                               * smooth(0.42f, 0.67f, wearNoise) * scuffHeight;
            const float stain = damp * dampFade * 0.081f + join * 0.071f + scuffs * 0.087f;
            const float shade = paper + grain - ink - stain;
            float r = 0.766f + shade, g = 0.690f + shade, b = 0.430f + shade * 0.78f;
            const float unscarredRed = r;
            scarColor(scars, metersX, metersY, wearNoise, r, g, b);
            pixel(image, x, y, r, g, b, kWallWidth);
            heights[y * kWallWidth + x] = paper * 0.018f + grain * 0.012f
                + ink * 0.006f - join * 0.0018f + (r - unscarredRed) * 0.012f;
        }
    }
    textures.wallpaper = upload(image, kWallWidth, kWallHeight);
    textures.wallpaperNormal = uploadNormals(heights, kWallWidth, kWallHeight,
                                             kWallMeters, kWallHeightMeters);
    image.resize(kSize * kSize * 3);
    heights.resize(kSize * kSize);

    for (int y = 0; y < kSize; ++y) {
        for (int x = 0; x < kSize; ++x) {
            const float u = float(x) / kSize, v = float(y) / kSize;
            const float mottling = (noise(u, v, 8, 131u) - 0.5f) * 0.030f;
            const float tufts = (noise(u, v, 96, 541u) - 0.5f) * 0.083f;
            const float yarn = (lattice(x, y / 3, kSize, 901u) - 0.5f) * 0.075f;
            const float speckle = (lattice(x, y, kSize, 827u) - 0.5f) * 0.062f;
            const float damp = smooth(0.57f, 0.81f, noise(u, v, 4, 1969u)) * 0.021f;
            const float shade = mottling + tufts + yarn + speckle - damp;
            pixel(image, x, y, 0.441f + shade, 0.397f + shade, 0.271f + shade * 0.76f);
            heights[y * kSize + x] = tufts * 0.024f + yarn * 0.016f
                                  + speckle * 0.009f - damp * 0.020f;
        }
    }
    textures.carpet = upload(image);
    textures.carpetNormal = uploadNormals(heights, kSize, kSize, 1.0f, 1.0f);

    for (int y = 0; y < kSize; ++y) {
        for (int x = 0; x < kSize; ++x) {
            const float u = float(x) / kSize, v = float(y) / kSize;
            const float gx = fract(u * 2.0f), gy = fract(v * 2.0f);
            const float edge = std::min(std::min(gx, 1.0f - gx), std::min(gy, 1.0f - gy));
            const float seam = 1.0f - smooth(0.007f, 0.016f, edge);
            const float bevel = 1.0f - smooth(0.012f, 0.034f, edge);
            const float pores = smooth(0.84f, 0.98f, lattice(x, y, kSize, 9021u));
            const float variation = (noise(u, v, 8, 523u) - 0.5f) * 0.027f;
            const float waterMark = smooth(0.63f, 0.87f, noise(u, v, 4, 6382u));
            const float shade = variation - seam * 0.23f - bevel * 0.030f - pores * 0.077f;
            pixel(image, x, y, 0.803f + shade - waterMark * 0.030f,
                  0.789f + shade - waterMark * 0.042f, 0.678f + shade - waterMark * 0.056f);
            heights[y * kSize + x] = -seam * 0.0024f - bevel * 0.0008f
                                  - pores * 0.00045f + variation * 0.004f;
        }
    }
    textures.ceiling = upload(image);
    textures.ceilingNormal = uploadNormals(heights, kSize, kSize, 1.0f, 1.0f);

    // Cream-coloured worn service tiles distinguish rare hard-floor rooms
    // from carpet while staying inside the muted yellow Backrooms palette.
    for (int y = 0; y < kSize; ++y) {
        for (int x = 0; x < kSize; ++x) {
            const float u = float(x) / kSize, v = float(y) / kSize;
            const float gx = fract(u * 2.0f), gy = fract(v * 2.0f);
            const float edge = std::min(std::min(gx, 1.0f - gx), std::min(gy, 1.0f - gy));
            const float seam = 1.0f - smooth(0.010f, 0.026f, edge);
            const float bevel = 1.0f - smooth(0.021f, 0.058f, edge);
            const float dirt = smooth(0.49f, 0.85f, noise(u, v, 8, 7103u));
            const float pores = (lattice(x, y, kSize, 7211u) - 0.5f) * 0.024f;
            const float shade = pores - seam * 0.18f - bevel * 0.025f - dirt * 0.075f;
            pixel(image, x, y, 0.565f + shade, 0.552f + shade, 0.456f + shade * 0.90f);
            heights[y * kSize + x] = -seam * 0.003f - bevel * 0.0012f + pores * 0.008f;
        }
    }
    textures.hardFloor = upload(image);
    textures.hardFloorNormal = uploadNormals(heights, kSize, kSize, 1.0f, 1.0f);
    glBindTexture(GL_TEXTURE_2D, 0);
    return textures;
}

void destroyTextures(Textures& textures) {
    const unsigned int ids[] = {textures.wallpaper, textures.carpet, textures.ceiling,
        textures.wallpaperNormal, textures.carpetNormal, textures.ceilingNormal,
        textures.hardFloor, textures.hardFloorNormal};
    glDeleteTextures(8, ids);
    textures = Textures{};
}

} // namespace br
