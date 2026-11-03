# Level 0 exploration and presence — 26 October 2026

The first stage keeps its yellow rooms, existing materials and short route to
the key and exit. New details help rooms feel recognizable and reward looking
into a side passage without adding another required objective.

## Landmarks and optional notes

Sparse seeded corners contain an unpowered fluorescent housing with broken
tubes, displaced ceiling panels, or damp wallpaper debris. They use small
static meshes and the existing material palette. Details remain close to the
floor and away from central walking routes, doors, hiding entrances and the exit.
They do not add collision obstacles or change lamps, room topology or navigation.

Two note corners are selected near the early and middle parts of the reserved
route, using existing connected branches one or two cells away. Each has a
small paper sheet and a different kind of damage. Approach within 1.8 metres,
look toward the paper and press **E** when **READ NOTE** appears. Walls block
inspection; door and hiding interactions retain priority.

Reading briefly displays two lines of environmental storytelling. The game
continues running while the text is visible. Notes are optional, remain in the
world, and can be read again. Their positions and kinds survive chunk streaming.
Pause freezes the display timer; returning to the entrance or changing levels
clears the current text. Notes do not summon Aranea, grant a key or lengthen the
approximately 123–138 metre mandatory route.

## A presence behind the door

A closed ordinary door can give two small handle pulls and a quiet mechanical
sound when the real, wandering Aranea is close on its far side. This is not an
extra actor or a scripted teleport. The door leaf and its collision do not move.

- The player must be 5–12 metres away and looking toward the door.
- Aranea must be on the opposite side within 4.5 metres of the door, at least
  7 metres from the player, with clear approaches and an obstructed sight line.
- The exit door, open doors, pursuit/search, hiding and death cannot trigger it.
- The first eligible event waits 12–20 active wandering seconds; subsequent
  events wait 40–65 seconds. Failed placement retries at most once a second,
  testing no more than eight nearby candidates.
- The two pulls finish in 0.85 seconds. Pause freezes them; opening or unloading
  the door, danger or changing levels cancels the animation.

The sound uses the existing spatial door cue and does not send an AI hearing
event. Unrelated ambient scheduling pauses for the short hint. No additional
light source, shader pass or persistent actor is introduced. Spawn clearance,
movement speed, perception and capture behavior remain unchanged.

## Verification

`tests/level0_landmark_tests.cpp` covers deterministic notes, short connected
branches, physical approaches, wall occlusion, mesh bounds/winding, streaming
and isolation from Fun/Parking. `tests/presence_tests.cpp` exercises real door
geometry, timing, pause/cancellation, bounded scans and handle-only rendering.
Both are included in the PowerShell, Make and CMake test targets.

`tools/verify-exploration.ps1` captures landmarks, note interaction, pause,
level changes and presence fixtures through the normal game loop. Developer
flags `--observation-preview 0|1`, `--auto-read-observation` and
`--presence-preview` are for repeatable checks; ordinary play uses none of them.

The release build passed both new native suites and the existing world,
progression and streaming suites. The landmark sweep checked 65 seeds and
130 short branches, with at most 20,304 vertices in a tested chunk. All 13
rendered exploration cases passed with zero OpenGL errors, including looking
down/away, pause, level travel, a settling handle and absence of a creature.
Normal Level 0-to-Fun and Fun-to-Parking exit checks also passed. Captured note
panels and floor details were inspected at 960x640.

Against commit `5977635`, all 18 sampled Fun/Parking world signatures and all
nine Level 0 collision signatures matched exactly. These are regression
checks, not an FPS benchmark or a complete manual playthrough.
