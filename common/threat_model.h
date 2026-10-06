#pragma once

#include "../world.h"
#include <filesystem>
#include <vector>

namespace br {

// User-supplied Smiler: original FBX mesh/rig, normalized to metres/Y-up/-Z front.
inline constexpr float THREAT_MODEL_HEIGHT = 2.86f;
// Cinematic clip is stabilized around this eye anchor (local X=Z=0).
inline constexpr float THREAT_EYE_HEIGHT = 2.60f;
inline constexpr int THREAT_BODY_MATERIAL = 20;
inline constexpr int THREAT_EYE_MATERIAL = 21;
inline constexpr int THREAT_RIDGE_MATERIAL = 22;

// Strict offline/cache validation, also used by the startup loader.
// Throws with an actionable asset error; does not replace the cached live model.
void validateThreatModelAsset(const std::filesystem::path& path);

// Time is simulation time (freeze it when paused); speed is metres/second.
// Locomotion clips blend by actual speed, including fast Search movement.
// scareAmount > 0 samples the source scream clip across normalized progress.
// stoopAmount blends to a separately articulated crouched version of each clip.
// locomotionPhase is a continuous [0,1) stride accumulator; negative uses time.
std::vector<Vertex> threatModelVertices(float animationTime, float speed,
                                      bool chasing, float scareAmount = 0.f,
                                      float stoopAmount = 0.f,
                                      float locomotionPhase = -1.f);

} // namespace br
