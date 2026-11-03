# BACKROOMS / Threshold v0.12 — Three-stage exploration

A first-person horror exploration game in **C++17 and OpenGL 3.3 core** for the Computer Graphics Fundamentals project. Explore three connected stages: worn yellow rooms, a deceptively cheerful party maze and a dark underground garage. Smooth first-person controls, stamina, interactive doors and spatial sound support a different objective and threat in each stage. Listen carefully, manage your surroundings and reach the elevator that leads to the surface.

![Aranea Membri in-game](assets/preview.png)

*The current four-legged Aranea Membri, captured in-game.*

## Levels

The normal route is **Yellow Rooms (Level 0) -> Level Fun -> Underground Parking -> City ending**.
You can also start any stage through **Escape -> Choose a Level**. Selecting a
stage starts it with fresh progress while keeping the seed and settings.
Checkpoints last for the current session and are not saved after closing the game.

### Stage 1 — Yellow Rooms / Level 0

Worn wallpaper, fluorescent lights and repeating corridors hide one four-legged
**Aranea Membri**. It hears movement, pursues you on sight and searches after
losing contact. Quiet ventilation, distant footsteps behind walls and rustling
from the hidden creature make empty rooms feel occupied. Break line of sight
and use hiding places to escape pursuit.

Find the brass key, follow the exit signal, open the red door with **E** and walk
through it to reach Level Fun. The connected route to the exit is about
**123–138 metres** from the start, with the key a few rooms before the door.

Rare fallen light fittings, broken ceiling panels and damp wallpaper debris
make some rooms recognizable. Two optional note corners sit along short side
branches: approach the paper and press **E** to read it. A closed door's handle
can briefly move when the real Aranea is wandering on the other side. These
details preserve the short main route and existing creature behavior.
See [Level 0 exploration and presence](docs/level0-exploration.md).

### Stage 2 — Level Fun

Orange walls, green carpet, party tables, cakes, balloons, garlands and wrapped
presents fill a connected party maze. Four yellow **Partygoers** with red balloons
freeze while you can see them and pursue when you look away. Contact is lethal:
the screen cuts to Game Over without a jumpscare, and **Restart Checkpoint**
returns you to the Level Fun entrance with fresh progress.

![A Partygoer in Level Fun, surrounded by party tables, cakes and balloons](assets/preview-level-fun.png)

*Level Fun and a Partygoer, captured in-game.*

Face a present and press **E** to search it for the exit key. The chance rises
after each miss, and the 15th searched present guarantees the key. Some boxes
release a squeaking mouse that attracts and speeds up the Partygoers. A rare
**Verity** pickup restores stamina and grants unlimited sprint stamina for the
current Level Fun run. Once you have the key, follow the exit signal, open the
door with **E** and walk into Parking.

Sparse deflated balloons, discarded cups, torn garlands and small winding music
boxes give rooms traces of an abandoned party. Quiet balloon creaks and brief
toy melodies come from actual nearby props. Moving Partygoers have soft spatial
footsteps that stop when they freeze under your gaze; nearby danger quiets the
decorative sounds.

With **C / clues** enabled, **PRESENT SIGNAL** points toward the nearest unopened
box within 32 metres, and **Tab** marks nearby boxes in pink. Opening a box removes
its marker; finding the key retires these clues. The signal does not reveal which
box holds the key. See [Level Fun atmosphere and exploration](docs/level-fun-upgrade.md).

Direct launch: `Backrooms.exe --demo --level-fun`.
See [Level Fun mechanics and checkpoint rules](docs/level-fun.md).

### Stage 3 — Underground Parking

Concrete columns, parked cars, service pipes and damp floor patches break up
a dark underground garage. Pairs of floating eyes approach through the unlit
bays. Aim an enabled **flashlight** at them to disperse them; simply looking
is not enough, and columns block the beam. The lit entrance and cabinet
approaches provide safe refuges. Contact in darkness triggers a jumpscare and
Game Over; restarting returns to the Parking entrance with fresh circuits.

![Underground Parking with cars, concrete columns and flashlight-lit bays](assets/preview-parking.png)

*Underground Parking, captured in-game with the flashlight enabled.*

Use the **CIRCUIT SIGNAL** and **Tab** map to find electrical cabinets **A, B and C**.
Approach each cabinet from the front and press **E** to restore its sector's
lights and make that sector safe. After all three circuits are online, follow
the exit signal, open the service door with **E** and walk through it. Crossing
the powered exit starts the elevator and city ending described below.

Sector letters now face all four sides of columns, and painted arrows identify
cabinet approaches. Sparse ventilation grilles, pipe leaks and coiled cables
add maintenance details. Quiet pipe ticks and drips originate at real fixtures;
ventilation can sound after its sector receives power. Each restored circuit
has its own relay/startup cue. Decorative sounds recede when the eyes are close.

The HUD shows **A / B / C ON or OFF**, highlighting the nearest unfinished
circuit; the same target is framed on the map. See
[Parking atmosphere and wayfinding](docs/parking-upgrade.md).

Direct launch: `Backrooms.exe --demo --parking`.
See [Parking mechanics and flashlight rules](docs/parking.md).

## Ending — Elevator to the surface

Restore all three Parking circuits, open the powered service exit with **E**,
and walk through it to start a **42-second cinematic**. The lift closes, rises
to a bright lobby, and opens the way to a daylight city street. The camera walks
outside and looks along the street before the **You found the way out** screen.

![The city ending with pedestrians, traffic, sunlight and cast shadows](assets/preview-city-ending.png)

*Actual in-game capture: the final street shot, with sunlight and moving traffic.*

- Twelve articulated pedestrians have varied proportions, clothing, hair and
  accessories. Six cars use compact, saloon and estate silhouettes, with rotating
  wheels and brake lights. Distinct cafe, bakery and bookshop displays line the road.
- A pedestrian crosses the zebra crossing while traffic smoothly brakes and
  waits; other passers-by briefly look toward the doorway. The same film clock
  drives the whole episode, including pause, seeking and replay.
- A visible sun casts shadows from buildings, trees, doors, people and cars.
  Static shadow maps are cached; animated shadows follow the models and freeze
  when the cinematic is paused.
- Lift sounds, an arrival chime, footsteps and generated city ambience accompany
  the sequence. **M** toggles sound; master volume applies.

This is an animated ending with an automatic camera. Preview it using **F9**
outside a jumpscare, or run:

```powershell
.\Backrooms.exe --ending-preview
```

During the film, **Esc** pauses/resumes and **Space** skips to the result.
At the result, **R** replays and **Enter** selects the highlighted button
(**Back to menu** by default). The buttons also support mouse clicks.
Returning to the menu preserves the current level and progress for this session.
The city uses generated geometry and sounds; it requires no additional library
or downloaded assets. See [sequence, controls and tests](docs/ending-elevator.md).

## Level selection and shorter first stage — 26 October 2026

Press **Escape**, then **Choose a Level** to select Yellow Rooms, Level Fun or
Parking with the mouse or Up/Down and Enter. Escape or Back returns to the
pause menu; Escape there resumes play. **Ctrl** is only used for crouching.
Selecting a level starts it at its entrance with fresh progress,
including when selecting the current level, while keeping the seed and settings.

The first level's exit is now within a **122.5–137.5 metre connected route**
from the start. The key remains three to five rooms before the door. HUD
straight-line distances can be smaller. See [level selection](docs/level-selection.md).

## Level 0 atmosphere update — 19 October 2026

The first level now has soft ventilation and settling sounds, occasional drips,
and rare muffled soil footsteps behind real walls. Quiet breath/drag cues come
from Aranea's actual position when it is hidden behind a nearby wall. Events
are spaced apart and fade out during pursuit so the real creature remains
audible. Pause and level transitions stop the Level 0 schedule.

Aranea now spawns 24–32 metres from the player, with at least 24 metres of
clearance from the run's initial player position. Spawns still require hidden,
reachable floor; the monster can subsequently walk toward the entrance.
See [atmosphere, spawn rules and verification](docs/level0-atmosphere.md).


## Aranea audio update — 13 October 2026

- **Soil footsteps:** separate walking and running banks use Fantozzi's CC0 sand recordings, with a soft impact and ground rustle. The hard clank and extra toe click were removed. Four variations per gait follow the creature's animated foot contacts and actual movement speed.
- **Sighting screams:** the creature screams when it sees the player, including when sound has already started a chase. Sustained visual pursuit repeats a call about every seven seconds; key pickup calls the creature to that location once and retains the 35% movement speed boost. A cooldown limits repeated detection calls.
- **Audible distant calls:** screams carry farther than footsteps and take priority when the mixer is busy. Stereo direction, distance, walls, closed doors, master volume and mute still affect them.
- **Reliable playback:** resuming from pause preserves cues triggered on the first frame. The complete 16-clip bank loads before gameplay; player footsteps and the capture sound retain their existing recordings.

The expanded audio tests, rebuilt game and five automated gameplay scenarios passed, covering walking, chasing, pause, key pickup and capture/restart. See [audio implementation and verification](docs/aranea-audio.md), [asset sources](assets/audio/aranea/README.md), and the bundled [sound preview](assets/audio/aranea/preview.wav).

## Play on this Windows computer

1. If you cloned or downloaded the source from GitHub, follow [Build from source](#build-from-source) below first. Then open **Backrooms.exe** in the built project folder.
2. Click **Enter the Backrooms**, or select it with **Up/Down** and press **Enter**.
3. Use **WASD** and the mouse. Press **Escape** to pause and release the cursor.

The GitHub repository contains source code and assets; the build produces `Backrooms.exe`. The local Desktop copy already contains this executable. Keep the seven `.glsl` files and the complete `assets/` directory beside it. No batch launcher is required. Project code generates environment textures, normal maps and ambient sound; footsteps use the bundled recordings credited in [THIRD_PARTY.md](THIRD_PARTY.md). The user's supplied **“Numbers” by Temporex** plays as background music. The game requires no photo assets or runtime downloads. If it cannot start, read `backrooms.log` and confirm that the graphics driver supports OpenGL 3.3.

## Controls

| Control | Action |
| --- | --- |
| Up/Down + Enter, or mouse click | Select and activate main/pause menu items |
| Enter, with the play/resume option selected | Begin / resume |
| Enter or click Restart Run, after being caught | Restart the same seeded building with fresh stamina, doors and creature state |
| WASD + mouse | Move and look |
| Left Shift | Sprint while stamina is available |
| Hold Ctrl | Crouch; lower the camera and walk quietly |
| E | Interact: doors, hiding places, Fun presents or Parking circuit cabinets |
| F | Toggle flashlight |
| C | Toggle flashlight-revealed clues |
| Tab | Toggle local map |
| H | Reopen the controls popup |
| V | Toggle minimal camera HUD |
| M | Mute / unmute all sounds |
| O, in the main/pause menu | Open settings |
| Left/Right, in settings | Adjust the selected setting; mouse minus/plus controls also work |
| F1 | Show performance and world diagnostics |
| F2 | Show wireframe geometry |
| F3 | Enable/disable changes to distant unseen rooms |
| R, during exploration | Return to the first room, keeping the seed |
| N, while paused | Start a different seeded building |
| F12 | Save a BMP screenshot to `captures/` |
| J | Trigger the debug jumpscare |
| F9 | Preview the elevator and city ending |
| Space, during the ending | Skip to the result screen |
| R, at the ending result | Replay the ending |
| Escape | Pause / resume; choose **Choose a Level** in the menu to start any of the three levels |
| Q, while paused | Quit |

Walking reaches **2.5 m/s**, Shift reaches **4.0 m/s**, and crouching reaches **1.1 m/s**. Acceleration and braking give movement weight. Holding Ctrl eases the eye height from 1.65 to 1.08 metres. Footsteps follow actual travel: crouching stays quiet, and wall pushing does not keep playing walking sounds. **M** mutes the complete sound mix.

Controls appear in a three-stage illustrated guide at the start of each run:
movement, exploration and survival. Click **Next** or press **Enter** to advance;
**Back** and the left arrow return to the previous page. **Begin Exploring** on
the final page closes it. **Escape** or the corner close button skips the guide;
**H** reopens it. The world and audio pause while it is open, and the cursor is
free. Nearby interaction prompts remain on the HUD.

Wall hiding pockets have metal ventilation covers: **E** enters behind the cover
or leaves it, and real gaps between the slats let you watch the corridor. Closed
covers block ordinary walking. Doors now have procedural brown wood grain. The
exit vestibule has near-black interior surfaces and no interior ceiling lamp;
the sign and approach remain readable.

A small brown mouse actively runs toward the player. Its first contact squeak
alerts the creature, then the mouse flees and cannot summon it again until the
mouse is reset. Quiet discovery sounds remain before the alarm. The tail is
joined into the body and has a closed tip. See [room details](docs/room-details.md).


Settings includes a live FPS readout and an **FPS COUNTER** toggle for gameplay. The toggle is saved with your other settings.
The **FPS LIMIT** setting defaults to **60** (30/60/90/120/144/240/unlimited). Menus and unfocused windows are capped at 30 FPS. This cap works independently of VSync and uses a blocking high-resolution timer on Windows. Existing settings files receive the 60 FPS default without resetting other preferences. Hidden benchmarks remain uncapped unless `--fps-limit N` is supplied. See [performance measurements and checks](docs/performance-2026-10.md).


Stamina starts at **100**, drains at **12 points per second** during actual sprint travel (about **8.3 seconds** of continuous sprinting), and recovers at **14 points per second** after **1.5 seconds** without sprinting. Exhaustion prevents sprinting until stamina recovers to **22**. Pausing freezes stamina. Doors swing smoothly through **90° in about 0.8 seconds**. They choose a hinge direction away from the player when opened from a fully closed position. Their collision follows the moving panel; an obstructed closing door reverses instead of trapping the player. Pausing also freezes door movement. Reserved connecting routes remain traversable.

Mouse look is smoothly eased, and restrained head bob follows actual movement. The default camera has light soft focus and a field-of-view change from **78° to 82°** during sprint travel. Version 0.10 reduces the blur radius from 1.7 to **0.8 reference pixels** at the image centre, roughly halving the previous softness while keeping existing settings. The interface remains sharp. Rooms have dim fluorescent lighting and deep shadows; use **F** to toggle the flashlight. V hides the ordinary navigation HUD for a cleaner camera view; it does not record video.

The clickable main and pause menus provide play/resume, settings, level selection, a new seed, a return to the entrance and quit. **Choose Level** is also available on Game Over. Escape or Back in the selector returns to the menu and keeps the game paused. Settings include **mouse sensitivity, master volume, music on/off, music volume, brightness, field of view, head bob, camera softness and VSync**, plus **Contact Shading** (0–100%, default 65%) and a reset-defaults option. Music volume defaults to **35%**, multiplied by master volume. Changes are saved to `settings.cfg` beside the executable. Head bob and camera softness can be set to zero. Press Escape in settings to return to the menu.

The yellow wallpaper and acoustic-ceiling style continues through classic rooms, repeating corridors, open halls, pillar halls, dead ends and occasional odd rooms with hard tiled floors. Layouts vary across **7.5, 15 and 22.5 metre** footprints under a uniform 3.2 metre ceiling. Procedural wall scars, peeling patches and damp marks combine with normal-mapped wallpaper, dry/damp carpet, ceiling tiles and hard flooring. The connected world continues generating as you walk. The older photo Nextbots and separate themed districts remain removed. Seeded recesses carved into existing yellow walls provide hiding places; the letter **H** marks them on the minimap (the **H** key opens help). Approach and press **E** to hide or leave. Break the creature's line of sight before entering. Hiding stops movement, lowers the camera, and suppresses the flashlight while allowing you to look out.

The Aranea animation update replaces the humanoid with the supplied four-legged creature. A staggered walking gait lifts one leg at a time while three support the body; running uses faster diagonal pairs. The body turns before moving, the head stays centered during locomotion and tracks independently while idle, the jaw moves, and step sounds follow all four foot contacts. The legs fold inward for doorways. See [Aranea animation and validation notes](docs/aranea-motion.md). The [earlier Smiler animation results](docs/smiler-motion.md) are historical and do not validate this replacement.

The **Aranea Membri** uses the actual 3D mesh from the user-supplied `rec-room-backrooms-entity-62-aranea-membri.zip`, including `source/Scene.glb` and its two carpet textures. All 135 mesh parts and 15,220 triangles are retained. The source has no rig or animation; the project adds four articulated leg chains, head/jaw movement, walk/run cycles and a capture pose. Offline conversion produces `assets/models/aranea/aranea.brm` and two texture caches, so Blender and Python are not needed to build or play. Source provenance and rebuild instructions are in the [model notes](assets/models/aranea/README.md). This replaces the previous Smiler; its earlier source and animation remain in local backups. After an initial **8-second grace period**, one creature can enter unseen, reachable space **24–32 metres** from the player, also outside a 24-metre radius around the initial player position. Walls and closed doors block its sight; obstructions reduce its hearing range. Crouching is quiet, sprinting is louder, and nearby door interactions can reveal your position.

The creature runs faster than walking speed but slower than the player's sprint. When contact is lost it searches the last sensed position, giving up after about **5 seconds** without seeing or hearing you. It navigates around walls and pillars, folds its legs through doorways and cannot catch through a closed door. Its corrected skull and body both face forward: it turns before moving along a new route segment, and its head stays centered while walking or running. Positional footfalls help you hear its direction during a chase.

Pivots now include actual leg transfers and contact sounds. Bounded support-foot
anchors reduce sliding, while tighter corrections preserve the folded doorway
pose. See [Aranea movement](docs/aranea-motion.md).

Contact or unobstructed proximity within **1.5 m** triggers a **2-second 3D jumpscare**, then **0.3 seconds of black** before Game Over. The camera turns toward the animated head over **0.20 seconds** with ease-out. The creature faces the camera and lunges to an eye-anchor distance of **0.90 m**, with a large close-up, reaching claws and head jitter. Random camera shake and roll fade over time, while FOV punches from **70° to 95°**. Edit `jumpscare.cfg` and restart to change duration, distance, timing and effects; `--jumpscare-config` selects an alternative file. **J** triggers a debug scare. See [jumpscare settings and implementation](docs/jumpscare.md).

Click **Restart Run** or press **Enter** with that option selected to restart the same seed. Restart restores stamina and resets the creature and doors; settings remain saved. **Main Menu** prepares a fresh run with the same seed and leaves it paused at the menu. Escape cannot resume a defeated run. Music pauses during capture and the game-over screen. **M** and master volume also apply to the jumpscare sound.

Ceiling fixtures have seeded **working, dying or off** states. Working lamps fluctuate gently, dying fixtures flicker more strongly, and off fixtures contribute no light. Each selected local light uses distance attenuation and cached shadows from walls, frames and doors. The flashlight provides a separate directional beam with filtered shadows, including the animated creature.

Sound uses the existing Windows **WinMM** output API, with no additional library. The stereo mixer pans world-space sources relative to the camera and attenuates them using their three-dimensional distance. The nearest active fluorescent fixture hums; seeded distant creaks and drips add ambience. The player uses the supplied freesound indoor footsteps uniformly on every floor; gait still controls cadence and volume. Aranea has separate ground walk/run contacts, a scream on seeing the player and lower pursuit calls, with four variants each. Footfalls follow the four animated feet; calls and steps use distance and wall/closed-door attenuation. See [Aranea audio](docs/aranea-audio.md) for the user's reference and processing. A subdued drone and occasional electrical stinger respond to environmental tension. Capture retains the user-supplied roar with a low impact, exclusive playback and controlled headroom. Ambient and door cues remain generated placeholders. All sound banks are cached before gameplay; source excerpts and conversion manifests are retained.

The soundtrack plays from `assets/audio/music/numbers-temporex.mp3`, converted locally from the supplied file for compatibility with this computer's Windows decoder. The original `numbers-temporex.m4a` is preserved in the same directory. Windows WinMM/MCI plays the music separately from positional environmental sounds, without another runtime library. Pause, **M**, or switching music off pauses the track; resuming keeps its playback position. Music volume can be lowered independently of footsteps and ambience. The generated audio-preview WAV contains the environmental mix, not the song.

The green **EXIT SIGNAL** shows distance and direction relative to your view. The exit is placed at a seeded random location. The key lies 3–5 connected rooms toward the entrance from that exit, keeping the remaining route to roughly 25–40 metres. Ordinary launches choose a fresh seed; `--seed` reproduces a layout and restarting preserves it. The exit starts **locked**. Follow the gold **KEY SIGNAL** and minimap marker to the seeded brass key. Walk close to collect it: the exit unlocks but stays closed, and the creature investigates the pickup location once. Follow the exit marker, **press E to open the door**, then walk through it to escape. Owning the key keeps the creature 35% faster but does not reveal your position or refresh its alert. The collected key persists while rooms stream; a new run resets it. See [key, hiding and audio details](docs/key-and-hiding.md). Pressing E alone does not end the game. Crossing the threshold now takes you directly to **Level Fun**, resets the player and clears the creature. Level Fun contains four yellow Partygoers with red balloons: they approach when unseen and freeze under your gaze. Contact is lethal without a jumpscare; restarting returns to the Level Fun entrance checkpoint. Search presents with E for its exit key (5% initial chance, +3 percentage points per miss, guaranteed by box 15), then manually open the exit and walk into the Parking checkpoint. See [Level Fun details](docs/level-fun.md). The local map shows the exit when it is in range.

To see the first clue, stand in the starting room, face its first north doorway, and aim the flashlight at the doorway's left side. Toggle F to compare.

## Features in version 0.12

- Three connected stages: Yellow Rooms with Aranea, Level Fun with Partygoers, and Parking with flashlight-sensitive eyes and three power circuits.
- A 42-second elevator-to-city ending with walking pedestrians, moving cars, a visible sun, cast shadows and generated street ambience.
- Seeded, connected yellow rooms, corridors, halls, pillars, dead ends and occasional tiled rooms.
- Smooth mouse look, gradual acceleration/braking, adjustable head bob, crouching, stamina and collision sliding.
- Animated hinged doors, a seeded wall-integrated exit and nearby key, and flashlight-revealed clues.
- The supplied textured 3D Aranea Membri with four-legged walk/run animation, independent head/jaw movement, 24–32 m spawns, sight/hearing detection, collision-aware pursuit, last-position searching and a timed loss of interest.
- A 3D capture jumpscare, Aranea ground footsteps and sighting/pursuit calls, game-over screen and same-seed restart.
- Procedural worn materials with UV mapping, mipmaps and normal maps.
- Working/dying/off fluorescent fixtures, per-light attenuation, cached six-face lamp shadows, PCF-filtered flashlight shadows, Blinn-Phong shading and distance fog.
- Half-resolution, depth-aware contact shading with adjustable strength, plus restrained paper, carpet and ceiling wear.
- An offscreen scene framebuffer with light soft focus, analog grain and vignette, followed by a sharp HUD.
- Stereo spatial hum, distant ambience, environmental tension, door sounds, universal player footsteps, wall-attenuated creature soil contacts and calls, and the supplied Temporex soundtrack.
- Main/pause menus and persistent camera, audio and display settings.
- Bounded room streaming, origin rebasing for distant travel, local map and rendering diagnostics.

Room loading keeps **25 playable chunks** and at most **24 prepared neighbours**. Each chunk has 6 × 6 cells, each 7.5 metres wide, for a **45 × 45 metre** chunk. One nearby chunk is prepared per frame and its mesh can upload before it becomes active. Unchanged nearby chunks can be reused on a turn back. GPU synchronization limits queued rendering work. Audio uses cached clips and at most 12 overlapping one-shot voices.

Initial loading, teleports and cache misses can still generate geometry synchronously. Read the measured results and their limits in [verification.md](docs/verification.md). The base world regenerates from its seed. Temporary room changes reset after their chunk unloads; at most 128 opened-door overrides are remembered, after which the oldest return to their seeded state when regenerated. “Endless” means continuing generation within finite numeric limits and bounded memory.

## Gameplay tuning and diagnostics

Edit `balance.cfg` beside the executable and restart to tune movement, stamina,
creature speed, hearing and memory without recompiling. The shipped values keep
the existing gameplay balance. Startup validates every supplied value and their
relationships; a missing or invalid file logs the problem and uses the complete
built-in defaults. `--balance-file PATH` selects an isolated configuration.
See [balance configuration](docs/balance.md) for keys, ranges and examples.

**F1** now includes process RAM, tracked OpenGL object counts, frame-time p95 and
slow-frame totals. `--performance-report PATH` writes JSON on shutdown, including
resource counts after automatic cleanup. Its histories are bounded to 720
memory/resource samples and 16 worst frames; frame histograms also have fixed
storage. Object counts do not measure GPU VRAM. See
[performance diagnostics](docs/performance.md) for the schema and measurement limits.

```powershell
.\Backrooms.exe --performance-report tests/artifacts/session-performance.json
.\tools\verify-performance.ps1 -SkipSoak # nine cycles / 27 level changes
.\tools\verify-performance.ps1           # also run 600 real seconds in Parking
```

The verifier checks level cleanup, bounded caches, resource plateaus and sustained
memory growth. Run it after building, with other instances of that executable
closed. It keeps timestamped reports and logs in `tests/artifacts/performance/`.

## Build from source

**CMake is the single source of build and test rules.** PowerShell, Make and the
launchers call the same dependency graph; they do not maintain separate lists
of C++ files or tests. Shared world, audio and rendering libraries are compiled
once per build configuration.

After `git pull`, close the game and use **Run.cmd**. It checks the CMake graph,
rebuilds changed dependencies, updates the root `Backrooms.exe` and launches it.
The EXE is ignored by Git, so pulling source alone does not update a running game.
The existing `Build.cmd` runs the native test suite as well.

On Windows, install CMake 3.21+ and MSYS2. In the **MinGW64 terminal**, install:

```sh
pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-glfw mingw-w64-x86_64-make mingw-w64-x86_64-cmake
```

The wrapper finds the compiler under `C:\msys64\mingw64`, `C:\msys64\ucrt64`,
on PATH, or through `BACKROOMS_CXX`. From the project folder:

```powershell
.\build.ps1 -IfNeeded                    # incremental game build
.\build.ps1 -Test                        # build and run native tests
.\build.ps1 -Test -GraphicsTest           # also exercise a real OpenGL context
.\build.ps1 -DebugBuild -Jobs 2           # separate debug build
```

GLAD is bundled. The Windows MinGW build statically links GLFW and the compiler
runtimes; Windows system libraries and an OpenGL 3.3-capable driver are required.
Settings remain beside the root executable and are never overwritten by building.

On Linux, install GCC, CMake, GLFW and OpenGL development packages, then use:

```sh
cmake --preset release
cmake --build --preset release
ctest --preset native
```

The CMake executable and runtime assets are in `build/release/bin/`. The target
`backrooms_deploy` also copies the executable to the project root. `make`,
`make test` and `make graphics-test` remain thin convenience wrappers.

[Build and test guide](docs/build-and-tests.md) explains test labels, graphics
checks, clean checkouts and CI. GitHub Actions runs native checks on Windows and
Linux, with a separate Mesa/Xvfb graphics job, when the workflow is pushed to the
repository. Local validation does not imply a hosted Actions run has completed.

## Source organization

See [Application architecture](docs/code-structure.md) for module responsibilities,
state transitions and the level reset lifecycle. CMake discovers
application `.cpp` files; every build wrapper uses that graph. `build.ps1 -Test`
also checks the screen flow. Move-only RAII owners release buffers, textures,
shaders, framebuffers and fences automatically; the window/context outlives all
graphics owners, including when initialization fails.

```text
Backrooms CGF Project/
  backrooms.cpp              Startup, asset-root selection and error reporting
  app/                       Game loop, input, menus, rendering and level lifecycle
  app/flow.h                 Exclusive application screen state
  app/gl_context.h           Window/context lifetime enclosing every GL owner
  app/performance.h / .cpp   Process memory sampling and bounded JSON reports
  world.cpp / world.h        Generation, streaming and collision
  vshader_backrooms.glsl     Model / view / projection vertex stage
  fshader_backrooms.glsl     Textures, local lighting, flashlight and fog
  vshader_lens.glsl          Fullscreen postprocess vertex stage
  fshader_lens.glsl          Soft focus, depth-aware contact shading, grain and vignette
  vshader_shadow.glsl / fshader_shadow.glsl   Flashlight and cached lamp depth maps
  fshader_ao.glsl            Half-resolution screen-space contact occlusion
  common/graphics.h         Shadow/AO targets and lamp face transforms
  common/gl_resource.h      Move-only GL owners and lifetime counters
  common/balance.h          Typed defaults, validation and frozen startup tuning
  common/performance.h      Fixed-size frame histograms and worst-frame samples
  common/gameplay.h         Movement speed and field-of-view settings
  common/movement.h         Acceleration and braking
  common/player_state.h     Stamina and smooth mouse-look state
  common/threat.h / .cpp    Single-creature sensing, navigation and behavior
  common/threat_model.h/.cpp Converted Aranea mesh, four-leg clips and capture pose
  common/ending.h           Cinematic timeline and automatic camera
  common/ending_city.h      Deterministic pedestrian and traffic motion
  common/ending_city_feet.h Planted feet, curb steps and continuous support motion
  common/ending_renderer.h  City, lift, articulated models and shadow passes
  common/ending_sun.h       Fixed near/far sunlight projections
  common/ending_ui.h        Cinematic captions, pause and result buttons
  common/settings.h         Validated persistent user settings
  common/audio.h / .cpp     Recorded steps, cached spatial mixer and Windows playback
  common/music.h / .cpp     Supplied soundtrack playback through Windows MCI
  common/                   Math, shader loading, textures, UI and GLAD
  include/                  Generated GLAD and Khronos headers
  tests/                    CPU tests and graphics verification evidence
  assets/audio/footsteps-player/   Supplied universal player contacts
  assets/audio/footsteps-monster/  Previous heavy contacts and fallback
  assets/audio/aranea/       Soil walk/run, sighting and pursuit banks
  assets/audio/music/        User-supplied Numbers by Temporex
  assets/models/aranea/      Supplied GLB, PNGs, animation bank and RGB texture caches
  tools/bake_aranea.py       Offline four-chain IK and head/jaw animation baker
  tools/aranea-rig.json      Original mesh-part joint and pivot mapping
  tools/prepare_aranea_textures.py   Offline texture-cache converter
  tools/prepare_aranea_audio.py     Offline audio-bank rebuild from retained sources
  tools/verify-aranea-audio.ps1     Automated audio gameplay scenarios
  tools/verify-performance.ps1     Repeated level transitions and timed soak
  assets/                   In-game preview and exported audio preview
  docs/                     Report draft, slides, demo and learning guide
  build.ps1                 Windows build and test
  settings.cfg              User preferences, created when settings change
  balance.cfg               Validated gameplay tuning, loaded once at startup
  jumpscare.cfg             Optional capture duration, camera, lunge and effect settings
  CMakeLists.txt            Shared build, dependencies, runtime assets and tests
  Makefile                  Thin CMake/CTest convenience wrapper
```

## Verification and demonstration

The resource/configuration/diagnostics upgrade passed 28 native and four real
OpenGL CTest cases, 48 full-game scenarios, 27 repeated level transitions and a
600-second real-time Parking soak.
Four isolated before/after scenes produced byte-identical screenshots and equal
non-timing profiles. These checks preserve behavior and verify cleanup; they do
not claim a frame-rate increase. Measured transition and ten-minute soak results
are in [performance diagnostics](docs/performance.md). Build and CI coverage are
explained in the [build and test guide](docs/build-and-tests.md).

After building, verify the ending and its lighting with:

```powershell
.\tools\verify-ending.ps1
.\tools\verify-ending-lighting.ps1
```

The first script checks 15 runtime cases, including the complete film,
pause/replay, sound toggling and the powered Parking exit. The second
compares shadowed and unshadowed captures and checks shadow caching,
moving objects, pause and resource cleanup. Evidence stays in the ignored
`tests/artifacts/` folder.

Read [verification.md](docs/verification.md) for executed checks and their limits. Automated captures can run without taking over the desktop:

```powershell
.\Backrooms.exe --hidden --demo --frames 120 --capture assets\test.bmp
.\Backrooms.exe --seed 42 --hidden --demo --frames 120
.\Backrooms.exe --demo --x 6.225 --z -265.8 --yaw -90
.\Backrooms.exe --audio-preview assets\audio-preview.wav
```

For this model replacement, `build.ps1 -Test` covers native behavior and model checks, and `tools/verify-aranea.ps1` exercises walk/run poses, head turns, door folding, live footfalls, pause, capture and restart. See [Aranea notes](docs/aranea-motion.md) for the current validation status. Install the rebuilt executable, `aranea.brm` and both Aranea `.rgb` caches together; the previous Smiler bank is not compatible.

For the current creature audio, run these from the project folder after building the tests:

```powershell
.\tools\verify-aranea-audio.ps1
.\build\windows-release\bin\audio_tests.exe --aranea-preview tests\artifacts\aranea-soil
```

The mixer preview contains walking, running, a detection scream at 14.32 metres and a pursuit call at 22 metres. It uses master volume 0.75 without normalization. The separate `assets/audio/aranea/preview.wav` demonstrates the source bank without world attenuation. Rebuilding those assets with `python tools/prepare_aranea_audio.py` requires NumPy only for offline processing; playing the game does not.

The 12-second stereo sound preview demonstrates carpet walking, damp-carpet crouching, hard-floor sprinting, left/right ambience and environmental tension. It mixes the bundled footstep recordings with generated sounds and does not record a playthrough. Supporting materials:

- [Course requirements and status](docs/requirements.md)
- [Report draft](docs/report-draft.md)
- [Earlier presentation](docs/presentation.html), a historical v0.8 deck to update before demonstrating v0.12
- [3–6-minute demo script](docs/demo-script.md)
- [Learning guide](docs/learning-guide.md)
- [AI assistance disclosure](docs/AI_USE.md)
- [Third-party licenses](THIRD_PARTY.md)

## Scope and remaining work

This version has three playable stages with distinct threats and a separate animated city ending. The city is a finite cinematic scene, not a fourth exploration level. There are no weapons, network play or saved gameplay sessions. Aranea uses bounded local pathfinding and a short memory, without complex tactics or a general-purpose navigation system. Settings persist, but world progress does not. Direct lamp, flashlight and city sunlight use shadow maps; small ambient fill remains, without full global illumination. Audible spatial playback uses stereo panning and distance attenuation plus direct-ray wall attenuation for creature sounds, without HRTF filtering or acoustic propagation; the creature's hearing uses a separate obstruction-based detection rule. Normal maps change shading, not geometry. The final powered exit starts the elevator-to-city cinematic and its replay/menu screen. Initial loading and cache misses remain synchronous, and procedural architectural motifs repeat.

This educational prototype includes substantial Codex assistance. Review and adapt the implementation, record student contributions and be able to explain it. The supplied syllabus's restrictions and disclosure requirements still apply. Team details, the official report template, defence date and final instructor rubric remain to be confirmed before an assessed submission. Nothing has been submitted to Moodle.

## Lighting and material update — 11 October 2026

The flashlight now casts filtered shadows from room geometry, swinging doors,
ventilation slats and the animated creature. Its emitter sits slightly below and
to the right of the camera, making nearby shadows visible. It returns to the
camera position when that offset would penetrate a wall.

Each of the selected ceiling lights has six cached 256px depth views. Walls,
frames, grilles and live door panels block its direct light; moving a nearby door
or changing nearby streamed geometry refreshes that light's cache. Lamp flicker
changes intensity without rebuilding the depth maps. Small ambient fill remains.

Contact Shading adds restrained screen-space occlusion in corners and joints.
Set it to zero in Settings to disable its rendering pass. Paper, carpet and
ceiling variations retain the existing yellow palette, layout and texture banks;
rare wet patches use lighting highlights rather than mirror reflections.

Follow-up fixes removed the repeating screen-grain grid and vertical contact-shading bands on angled walls. Grain now uses integer hashing, while contact shading uses matching depth texels and pixel centers with depth-aware upsampling. The existing wallpaper textures and visual style are preserved.

See [graphics-update.md](docs/graphics-update.md) for measurements and limitations.
Use `build.ps1 -Test -GraphicsTest` for native tests and rendered shadow/AO probes.
