#pragma once

#include "../world.h"
#include <array>
#include <filesystem>
#include <vector>

namespace br {

// User-supplied Aranea Membri mesh, articulated offline in metres/Y-up/-Z front.
inline constexpr float THREAT_MODEL_HEIGHT = 2.40f;
// Cinematic clip is stabilized around this eye anchor (local X=Z=0).
inline constexpr float THREAT_EYE_HEIGHT = 1.75f;
inline constexpr int THREAT_BODY_MATERIAL = 20;
inline constexpr int THREAT_EYE_MATERIAL = 21;
inline constexpr int THREAT_RIDGE_MATERIAL = 22;

// Strict offline/cache validation, also used by the startup loader.
// Throws with an actionable asset error; does not replace the cached live model.
void validateThreatModelAsset(const std::filesystem::path& path);

// Time is simulation time (freeze it when paused); speed is metres/second.
// Locomotion clips blend by actual speed, including fast Search movement.
// scareAmount > 0 samples the capture clip across normalized progress.
// stoopAmount blends to a separately articulated crouched version of each clip.
// locomotionPhase is a continuous [0,1) stride accumulator; negative uses time.
std::vector<Vertex> threatModelVertices(float animationTime, float speed,
                                      bool chasing, float scareAmount = 0.f,
                                      float stoopAmount = 0.f,
                                      float locomotionPhase = -1.f,
                                      float headYaw = 0.f, float headPitch = 0.f);

// Feet are ordered front-left, front-right, rear-left, rear-right.
struct ThreatFootContacts { std::array<float,4> feet{{1.f,1.f,1.f,1.f}}; };
ThreatFootContacts threatFootContacts(float speed,float locomotionPhase);
// Full locomotion-cycle travel in metres, shared by pose blending and movement.
float threatStrideLength(float speed);

} // namespace br
