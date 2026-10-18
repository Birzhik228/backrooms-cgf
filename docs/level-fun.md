# Level Fun progression update

Entering Level Fun establishes a session checkpoint. Death restarts at its
entrance with the same seed; presents, key, doors and Partygoers reset. It does
not return to Level 0. Checkpoints are not saved across application launches.

Partygoers still freeze while visible, but contact within 0.65 m is now lethal.
Walls, closed doors and furniture block contact. Death cuts to black and Game
Over without a jumpscare; choose Restart Checkpoint.

Face a nearby present and press E. Searched boxes disappear completely,
including collision, in both mesh detail levels. A dedicated paper tear and
cardboard-opening sound plays. Each box can be searched once. The
first key chance is 5%, with 3 percentage points added after every miss; the
15th box guarantees a key. Rolls are seeded and cannot be rerolled by leaving
and revisiting rooms. After finding the key, follow EXIT SIGNAL and press E
at the locked exit, then walk through it. This now leads to the third stage,
[Underground Parking](parking.md), with a fresh checkpoint and circuit objective.

Each non-key present has a separate seeded 20% mouse chance. A mouse squeaks
once on release, flees for up to eight seconds, and alerts all four Partygoers
to the release position for six seconds. Gaze freeze still applies. Mice count
as misses for key pity; they never consume a winning key roll. No ambient mice
spawn on Level Fun, and released mice cannot respawn or repeat their alarm.

Both levels now use a red painted-metal exit leaf with a silver panic bar and
kick plate; ordinary doors retain their oak material.

The key in Level 0 and its 35% Aranea speed boost are unchanged.

## Previous rendering notes (historical)

# Level Fun

Walking through the unlocked Level 0 exit now immediately starts Level Fun.
The key and exit rules in Level 0 are unchanged. The transition creates a new
world, unloads old GPU chunks, clears door/key/mouse meshes, invalidates lamp
shadow caches, and resets player movement, stamina, creature, mouse, audio and
jumpscare state. It keeps the seed and user settings. There is no intervening
victory menu.

Level Fun is an endless, connected party interior inspired by the user's visual
reference: orange walls, dark green carpet, acoustic ceiling panels, blue chairs,
off-white patterned tablecloths, coloured balloons and triangular bunting.
Table settings vary: some seats are empty, some plates have cake slices, and
about one table in three has a central birthday cake. Cups also vary. Some corners hold wrapped gifts.
All props are native geometry using the existing OpenGL lighting, normal maps,
shadows and post-processing; no additional asset or library is required.

`World(seed, Level::Fun)` selects the new geometry generator. It shares the
seeded connected room topology and the 25-resident, 24-prepared chunk streamer
with Level 0. Furniture density, secondary tables, palette, balloons, gifts and
bunting vary deterministically by cell hash. Clear crosses between cell centres
and doorways preserve the connected routes. Tables, chairs, gifts and walls
have collision; overhead decoration stays clear of a standing player's head.
Streaming back to an unmodified chunk recreates the same props.

Aranea and the mouse cannot spawn, update or trigger capture in Level Fun,
including via debug J or automated capture fixtures. Four Partygoers now inhabit
the level (see below). Ambient room audio and player footsteps remain; old creature voices
are cleared at transition. Presents are now interactive; see the progression update above. The minimap
shows the Fun exit, and the HUD provides its distance and bearing.

R returns to the current level's entrance. New Seed creates another building
on the current level. Pause, settings, flashlight, crouch, sprint and map remain
available. The illustrated controls guide has Level Fun-specific pages.

For a development preview, use `Backrooms.exe --demo --level-fun`; normal play
reaches it through the Level 0 exit.

`tests/fun_tests.cpp` checks three seeds, clear connected routes, furniture
collision, geometry bounds and winding, required prop materials, deterministic
streaming and isolation from Level 0. It runs with `build.ps1 -Test`, CTest and
Make. `tools/verify-fun.ps1` checks the actual transition, different seeds, table
details, streaming, guide, pause and absence of entities beyond spawn grace.
Rendered evidence is saved under `tests/artifacts/fun/`.

Verified on 16 October 2026: all ten native test suites and all eight Level Fun
rendered scenarios passed with zero OpenGL errors. The party-room, table-detail
and actual exit-transition captures were visually inspected. The 600-frame
entity-guard run kept the creature dormant, the mouse inactive and all creature
audio/capture counters at zero, including forced/debug capture inputs. It
averaged about 34 FPS at 1280x800 on this machine's Intel UHD GPU; initial chunk
loading causes a longer first frame. Performance varies with room density and
view direction. These checks are automated, not a human playtest.

## Visibility and distant detail optimization — 16 October 2026

Static meshes now have cell-sized draw sections with bounds computed from
the actual triangles (including geometry crossing cell boundaries). Camera
and shadow passes reject sections outside their frusta; multi-draw batches
the remaining ranges. This is conservative frustum culling, not wall/portal
occlusion. Level 0 also benefits without changing its generated geometry.

Level Fun has a second, render-only mesh beyond 24 metres from a section's
nearest bound. It simplifies round props and removes tiny cherries, candles
and chair slats. Tables, chairs, plates, cakes, balloons, walls and lighting
remain; nearby geometry and all collision/interaction data are unchanged.
Cached shadows use full-detail sections so camera movement cannot leave stale
LOD shadows. The second mesh increases resident mesh memory and generation
work; initial loading can still hitch. This change targets steady gameplay.

At 1280x800, FOV 94, Intel UHD, the same stationary view dropped from 625,002
to 243,704 submitted scene triangles; walking averaged 835,280 to 364,290.
240-frame hidden runs excluding 60 warmup frames measured approximately
33–35 to 37–39 FPS stationary and 28.5 to 34 FPS walking. Level 0 measured
46.2 to 48.5 FPS. These are short local benchmarks, not guaranteed frame rates.
Profiles and captures are in `tests/artifacts/performance/`.

All ten native suites passed, including distant mesh bounds/material checks;
all eight Fun rendering scenarios and the Level 0 Aranea walk-support check
passed with zero GL errors. The settings roundtrip test required an unrestricted
run because temporary-file writes failed in the sandbox. The final EXE was
rebuilt and its Level Fun benchmark completed successfully.


## Partygoers

Four procedural yellow Partygoers inhabit Level Fun. Their sack-like costumes,
painted red eyes/smiles and red balloons follow the user's image reference;
ordinary scene materials and flashlight shadows keep them in the same visual
style. No external model files are required.

Each actor independently freezes position, turning, gait and balloon animation
when a sampled part of its silhouette is in the camera's field of view with an unobstructed
height-aware ray through the walls. Camera pitch, aspect ratio and FOV matter;
low tables do not hide a head. Looking away resumes slow pursuit at 1.95 m/s.
Routing uses a bounded 256-cell search, cached per actor, and swept collision.
Actors avoid one another, spawn at safe unseen cell centres and relocate only
when far away and unseen. Nearby occupied chunks are protected from room shifts.
Pause, the guide and restart suspend or reset their state.

Contact currently stops the approach at roughly 0.95 m: no damage, death,
scream, flash, camera takeover or jumpscare is added. Level 0's Aranea mechanics
remain unchanged. `--no-threat` also disables Partygoers.

`--partygoer-preview --demo` puts one actor in front of the starting camera;
`--partygoer-check --hidden --demo --frames 180` additionally looks away at
frame 60 and back at frame 120. Native tests cover freeze/resume, pitch, wall
occlusion, pause, contact, four-actor spawning, collision, mesh budget and reset.


Verified: all eleven native suites, eight Level Fun scenarios, five dedicated
Partygoer rendered scenarios and the Level 0 Aranea walk-support scenario passed.
The final portrait was inspected after reducing costume texture contrast.
The four-actor population benchmark measured about 38.5 FPS at 1280x800/FOV94
on Intel UHD after 60 warmup frames, compared with the previous ~39 FPS view.
This short benchmark does not cover every camera angle or a crowded close-up.
Artifacts are under `tests/artifacts/partygoer/`; the rebuilt executable includes
the new AI, procedural models and gaze checks.


17 October update: gaze samples rotate with the actor, and a sample can no
longer freeze an actor through a wall when the body at that height is occluded.
A regression places the actor close behind an opaque wall where the old wide
samples protruded through it. Speed is now 1.95 m/s (previously 1.65).
Sparse table settings use identical seeded choices in both detail levels;
furniture positions, chairs, collisions and connected routes are unchanged.


## Shorter render distance � 17 October 2026

The camera draw limit is now 45 m instead of 110 m on both levels. Scene
sections whose nearest horizontal bound is beyond that radius are skipped;
shadow passes retain their light-space bounds. Existing fog is preserved nearby
and fades fully to the black clear colour from about 28 m to 43 m, before the
cutoff. Partygoer gaze checks respect the fully obscured distance. World
streaming, collision and navigation ranges are unchanged. The shared constants
are in `common/render_limits.h`.

A stationary 1280x800/FOV94 Level Fun comparison, with actors disabled and the
first 60 of 240 frames excluded, reduced scene triangles from 204,680 to 58,164
(about 72%). FPS was essentially unchanged: 42.5 before, 41.5 after. This reduces
geometry work but does not remove the remaining lighting/pixel rendering cost.
Profiles and the inspected screenshot are in `tests/artifacts/draw-distance/`.
The rebuilt executable passed native Partygoer tests and rendered gaze,
streaming and Level 0 Aranea checks.


Progression verification: all 11 native suites passed. Rendered checks passed
for contact death, checkpoint restart, present searching, unlocked exit
completion, locked exit rejection and the Level 0 transition, with zero OpenGL
errors. Opened-present, Game Over and completion captures were inspected.


## Verity and mouse rush

Mouse alarms raise every Partygoer's movement speed from 4.875 to 7.5 m/s for
six simulation seconds. Looking at an actor still stops its movement; pause
freezes the timer, and speed returns to normal when the alarm expires.

Each searched present also rolls an independent 5% Verity bonus. It can appear
alongside the key or mouse and never consumes either outcome or key pity.
Walk within 0.75 m with a clear path to collect the floating yellow smiley.
It restores stamina immediately and prevents sprint drain on the current Level
Fun run. The HUD reads VERITY / INFINITE. Death/checkpoint restart, a new seed,
or changing levels clears the bonus; returning within the same level retains it.

The lightweight procedural sphere, oval eyes and smile follow this visual
reference: https://ko-fi.com/s/e543c81319 . No third-party model is bundled.
Drops survive room streaming; collected drops cannot grant the reward twice.

Presents remain searchable after acquiring the key. Further searches keep the
key, never award duplicate keys, and retain the 20% mouse and independent 5%
Verity chances. Both Partygoer speeds were multiplied by 2.5; gaze freeze and
the six-second alarm duration are unchanged.
