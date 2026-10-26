# Elevator to the surface — test ending

This is an original 42-second in-engine cinematic prototype. It starts after
walking through Parking's powered service exit, once circuits A, B and C are
restored. It can also be previewed without completing a level.

## Sequence

- The metal lift doors close on the dim garage.
- The cabin rises with a low motor sound, small vibration and a floor indicator.
- An arrival chime precedes the doors opening onto a bright, empty lobby.
- The camera walks automatically toward the outer door. Faint outdoor ambience
  suggests that the surface is close.
- The hinged exit opens onto a daylight city street. The camera walks onto the
  pavement and looks down the street as people walk and cars pass in both lanes.
  Apartment buildings, shops, trees, streetlights and a distant skyline surround
  the scene; city ambience continues until the final fade.
- A result screen reads: **You found the way out.** It offers replay and
  return to the menu. It does not silently restart the player's run.

The player's movement and ordinary world simulation are suspended while the
cinematic owns the camera. Normal gameplay controls do not steer the shot.
The setting is a compact separate scene; it does not add another streamed level.
Meshes and synthesized sound clips are cached instead of rebuilt every frame.
No additional library, downloaded recording or imported asset is required.

## Sunlight

A visible sun above the skyline supplies the same warm directional light used
for cast shadows. Buildings, trees, street furniture, doors, people and cars
cast and receive shadows; limbs and wheels use exactly the same transforms in
the depth and colour passes. Blue sky fill keeps shaded surfaces readable.

Two fixed shadow projections cover the nearby street and the distant city.
Static geometry is rendered into its depth maps once. Separate moving-object
maps refresh only when the film clock changes, so pause does no shadow updates.
Near maps use 2048 pixels; the distant moving-object map uses 1024. A small PCF
filter softens shadow edges, and the projections do not follow the camera.
All four textures and the framebuffer are released on return to the menu.

Run `tools/verify-ending-lighting.ps1` for rendered sunlight tests and comparison
captures. It checks actual occlusion against identical unshadowed frames,
static caching, moving/paused shadows, replay and resource recreation.

Sunlight validation on 26 October 2026 passed on Intel UHD: zero OpenGL errors,
matching paused/replayed images, stable static caches and moving actor shadows.
The warmed 120-frame city draw measured 3.77 ms/frame at 1280x800 in the local
standalone renderer test (not an end-to-end gameplay benchmark). All 13 full-game
ending checks passed again; the sun is visible below the cinematic border in
the final street shot.

## Preview and controls

Press **F9** to preview the ending, or launch:

```powershell
.\Backrooms.exe --ending-preview
```

During playback, **Escape** pauses/resumes, **Space** skips to the result, and
**M** mutes/unmutes audio. At the result, **R** replays and **Enter** returns to the
main menu. Master volume applies to the cinematic sounds. Returning to the menu
preserves the current level and its progress.

For deterministic captures, `--ending-time N` starts a preview at that second
(0–42). Hidden previews use a fixed 1/60-second simulation step:

```powershell
.\Backrooms.exe --hidden --ending-time 26 --frames 2 --capture reveal.bmp
```

## Verification

`tools/verify-ending.ps1` covers nine visual milestones, the complete sequence,
pause/resume/skip/replay/menu controls, city ambience mute/unmute, and the actual
powered Parking exit (13 cases).
Captures, logs and JSON results are written to `tests/artifacts/ending`.
The timeline suite also checks deterministic city actors, lane/sidewalk bounds,
vehicle spacing and camera continuity. Audio tests cover isolation, pause, mute
and the generated street sound.

Validated on 26 October 2026: all 13 runtime cases passed with zero OpenGL errors
and frozen gameplay during the film. The complete 42-second sequence, natural
Parking exit and city sound off/on controls passed. Native motion and cinematic
audio suites passed, as did the existing audio regression suite. City captures
at 26, 32 and 37 seconds were visually inspected. This is a lightweight stylized
city prototype with generated geometry and placeholder street sounds.
