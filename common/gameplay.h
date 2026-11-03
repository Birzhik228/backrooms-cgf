#pragma once
#include "balance.h"

namespace br {
// Metres per second. A measured walking pace suits quiet exploration.
inline const float& WALK_SPEED = balance().playerWalkSpeed;
inline const float& SPRINT_SPEED = balance().playerSprintSpeed;
inline const float& CROUCH_SPEED = balance().playerCrouchSpeed;
constexpr float WALK_FOV = 78.0f;
constexpr float SPRINT_FOV = 82.0f;
} // namespace br
