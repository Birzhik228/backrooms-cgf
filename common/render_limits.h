#pragma once
namespace br {
// Camera rendering only; world streaming, navigation and collision stay intact.
inline constexpr float SCENE_VIEW_DISTANCE = 45.f;
inline constexpr float SCENE_FOG_END = SCENE_VIEW_DISTANCE - 2.f;
}
