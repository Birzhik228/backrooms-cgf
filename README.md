# BACKROOMS / Threshold v0.12

A first-person horror exploration game built with **C++17 and OpenGL 3.3** for the Computer Graphics Fundamentals project. Explore procedural Backrooms, listen for approaching creatures, manage stamina and find a way to the surface.

The game uses a custom renderer and gameplay code, with GLFW for windows/input and GLAD for OpenGL loading. Rooms, lighting, interactions, creature behavior and the final cinematic are handled by the project itself.

[Play](#play) · [Levels](#levels) · [Controls](#controls) · [Settings](#settings-and-configuration) · [Build](#build-from-source) · [Code](#project-structure) · [Tests](#tests-and-diagnostics)

## Features

- **Seeded, endless exploration:** yellow rooms, corridors, pillars, open halls, dead ends and occasional unusual spaces. Nearby chunks are loaded and distant ones released as the player travels.
- **First-person movement:** smooth mouse look, acceleration and braking, walking, crouching, sprinting, stamina and adjustable head bob.
- **Lighting and materials:** flickering fluorescent fixtures, working/dying/off lights, an interactive flashlight, shadows, stained wallpaper, carpet and ceiling textures with normal maps.
- **Physical interactions:** animated doors with collision, keys, hiding recesses, optional notes, presents and electrical cabinets.
- **Spatial sound:** fluorescent hum, recorded footsteps, creature calls, distance attenuation, stereo positioning, wall muffling and level-specific ambience.
- **Presentation:** main/pause menus, an in-game field guide, configurable camera effects, capture sequences and an animated city ending.

## Play

The complete playable version targets **Windows** and requires a graphics driver supporting **OpenGL 3.3**.

1. Open **Backrooms.exe** in a built copy of the project.
2. Start exploring from the main menu; the field guide explains movement and the current level's objective.
3. Use **Escape → Settings** to adjust sound, camera and display preferences. **H** reopens the guide.

Keep `assets/`, the `.glsl` shaders, `balance.cfg` and `jumpscare.cfg` beside the executable. To share a built copy with another player, send these together; the EXE alone is insufficient. The standard Windows build links GLFW and the compiler runtime statically, so players do not need MSYS2 installed.

A GitHub clone contains source code and assets; **the EXE is not included**. Follow [Build from source](#build-from-source) below. If startup fails, check `backrooms.log` beside the executable.

## Levels

**Yellow Rooms → Level Fun → Underground Parking → City ending**

### Yellow Rooms / Level 0

Find the brass key, open the exit with **E**, then walk through the doorway. The rooms repeat and branch, with optional notes, clues and a local map to help exploration.

**Aranea Membri** is a four-legged creature that wanders, investigates sounds and chases when it detects the player. Sprinting and nearby interactions can reveal your position. Break line of sight, move quietly and enter a hiding recess when it cannot see you. Its model has walking/running cycles, head movement and a close-up capture animation.

![Aranea Membri in-game](assets/preview.png)

### Level Fun

Search presents among party tables, cakes and balloons to find the exit key. **Partygoers** freeze when visibly observed and chase when you look away. A mouse released from a present can alert them and temporarily increase their speed. Contact triggers a jumpscare and Game Over.

Some presents drop **Verity**, a small yellow smiley. Look at it within 1.2 metres and press **E** to collect it. It restores stamina and grants unlimited stamina for the current Level Fun run, with a short pickup song. Partygoer footsteps come from their actual movement, stop when they freeze, and briefly lower the background music when nearby.

![A Partygoer in Level Fun, surrounded by party tables, cakes and balloons](assets/preview-level-fun.png)

### Underground Parking

Restore circuits **A, B and C** at their electrical cabinets, then open the powered service exit. The HUD and local map help track the remaining circuits.

Floating eyes approach through the darkness. Aim the flashlight at them with a clear line of sight to drive them away. The entrance, cabinet areas and restored sectors offer safe light; cars and concrete columns interrupt visibility. Pipe sounds, ventilation and electrical relays replace the party ambience.

![Underground Parking with cars, concrete columns and flashlight-lit bays](assets/preview-parking.png)

### Elevator to the surface

The final exit starts an in-engine cinematic: the lift closes, rises and opens into a lobby, then the camera passes through the outer door onto a city street. Pedestrians walk, cars pass and stop at a crossing, and sunlight casts shadows across the buildings and moving actors.

The city is a scripted ending rather than another free-roam level. **Escape** pauses the sequence, **Space** skips it, and the result screen offers replay or return to the menu.

![The city ending with pedestrians, traffic, sunlight and cast shadows](assets/preview-city-ending.png)

All screenshots are captured in-game.

### Progress and restarts

- Completing a level leads to the next stage. After capture, restart begins the **current level** again with the same seed and fresh level progress.
- **R / Return to First Room** while alive returns to the entrance while retaining collected items, opened doors and restored circuits.
- **Escape → Choose a Level** opens any of the three playable stages with fresh progress and the current seed.
- Settings are saved locally. **Gameplay progress has no disk save** and is lost when the application closes.

## Controls

| Key | Action |
| --- | --- |
| WASD + mouse | Move and look |
| Hold Shift / Ctrl | Sprint / crouch |
| E | Interact with doors, hiding places, presents, Verity, notes and cabinets |
| F | Toggle flashlight |
| C / Tab | Toggle clues / map |
| Escape | Pause, settings and level selection |
| H | Open the controls guide |
| M | Mute / unmute sound |
| R | Return to the entrance; restart after defeat |
| N, from the paused menu | Start the current level with a new seed |
| F1 | Performance diagnostics |
| F9 | Preview the ending; Space skips it |
| F12 | Save a screenshot in `captures/` |

Menus support mouse selection and arrow keys/Enter. During the ending, controls operate the cinematic; normal movement is suspended.

## Settings and configuration

The settings menu includes mouse sensitivity, master volume, music volume/on-off, brightness, field of view, head bob, **Camera Softness** (blur), **Contact Shading** (ambient occlusion), VSync, an FPS counter and an FPS limit. Camera softness and head bob can be reduced to zero. The music toggle is independent of the session-wide **M** mute.

| File | Purpose |
| --- | --- |
| `settings.cfg` | Local player preferences, saved by the settings menu and excluded from Git. |
| `balance.cfg` | Movement, stamina and creature tuning; edit supported values and restart the application. |
| `jumpscare.cfg` | Capture duration, camera turn, lunge, shake, FOV effects and audio options. |

Balance values are validated together at startup. An invalid file produces a diagnostic in `backrooms.log` and falls back to built-in defaults. See the [balance guide](docs/balance.md) for keys, ranges and examples.

## Build from source

### Windows

Install **Git** and **MSYS2**. In the **MSYS2 MinGW64 terminal**, install the C++ compiler, GLFW, make and CMake (3.21 or newer):

```sh
pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-glfw mingw-w64-x86_64-make mingw-w64-x86_64-cmake
```

Clone the repository from a terminal where Git is available:

```sh
git clone https://github.com/Birzhik228/backrooms-cgf.git
cd backrooms-cgf
```

From the cloned folder in **PowerShell**:

```powershell
.\build.ps1 -IfNeeded
.\Backrooms.exe
```

The build produces `Backrooms.exe` in the project root. **Run.cmd** provides a double-click build-and-launch shortcut when the development tools are installed. Close the game before rebuilding. After pulling source changes, rebuild to update the executable and runtime assets.

`CMakeLists.txt` is the shared source of build rules; PowerShell and Make invoke it. The normal Windows build does not need Python or Blender: converted creature assets are already included.

### Direct CMake / Linux development

With a C++17 compiler, CMake, GLFW development files and OpenGL available:

```sh
cmake --preset release
cmake --build --preset release
ctest --preset native
```

This places the executable and runtime files in `build/release/bin/`. Linux build/test jobs are configured in CI, but **live audio output and soundtrack playback currently use Windows WinMM/MCI**. A successful Linux build does not provide the complete Windows audio experience.

See the [build and test guide](docs/build-and-tests.md) for alternate compilers, build directories, Make targets and headless graphics checks.

## Project structure

| Path | Responsibility |
| --- | --- |
| `backrooms.cpp` | Startup, command-line options, configuration and sound-bank loading. |
| `app/` | Game loop, player state, menus, interactions, audio coordination, rendering and level transitions. |
| `world.h` / `world.cpp` | Seeded generation, chunk streaming, collision and persistent in-session world state. |
| `common/` | Movement, creature AI/animation, audio mixing, materials, UI, graphics resource owners and cinematic helpers. |
| `*.glsl` | Scene lighting, shadows, ambient occlusion and camera post-processing shaders. |
| `assets/` | Supplied models, prepared animation/texture data, audio and README screenshots. |
| `tests/` | Native/rendered tests and saved reference scenes. Generated evidence goes into ignored `tests/artifacts/`. |
| `tools/` | Runtime verification scripts, profiling helpers and offline asset-preparation tools. |
| `docs/` | Architecture, mechanics, build details, verification notes and course drafts. |
| `.github/workflows/ci.yml` | Automated Windows/Linux builds and a separate software-OpenGL test job. |

Start with `app/game_loop.cpp` for frame order, `app/game_play.cpp` for gameplay phases, `app/game_interactions.cpp` for objects and `app/game_render.cpp` for drawing. The [code guide](docs/code-structure.md) explains their boundaries and reset behavior.

## Tests and diagnostics

Run these from the project root on Windows:

```powershell
.\build.ps1 -Test                     # Logic, generation, audio and lifecycle tests
.\build.ps1 -GraphicsTest             # Rendering checks; requires OpenGL 3.3
.\tools\verify-fun.ps1                # Full-game Fun interactions and progression
.\tools\verify-performance.ps1 -SkipSoak  # Repeated level changes and resource checks
```

Native and graphics suites can also be combined with `.\build.ps1 -Test -GraphicsTest`. Run full-game verification scripts one at a time per project copy because they share `backrooms.log`.

The merged build checked on **3 November 2026** passed **28 native tests, four OpenGL tests and 50 runtime scenarios** locally on Windows/Intel UHD. Nine unchanged-scene comparisons matched saved images and non-timing profiles. These results describe that local build, not a compatibility guarantee for every PC. GitHub Actions is configured to run builds/tests on pushes and pull requests; its results are separate from the local checks.

Press **F1** to inspect frame timing, memory and graphics resource counts. To save a diagnostic session:

```powershell
.\Backrooms.exe --seed 12071998 --performance-report tests/artifacts/session-performance.json
```

The report is written when the game exits. Streaming keeps a bounded set of nearby chunks, graphics resources use automatic ownership, and the frame limiter avoids unnecessary uncapped rendering. See [performance diagnostics](docs/performance.md) for repeated transitions, long sessions and interpretation of the counters.

## Troubleshooting

| Problem | What to check |
| --- | --- |
| `Backrooms.exe` is missing after cloning | Build the source first; executables are excluded from Git. |
| The game reports missing shaders, textures or sounds | Keep runtime assets beside the executable and inspect `backrooms.log`. Restore missing repository files; rebuilding copies runtime files into the build output. |
| OpenGL initialization fails | Install a graphics driver that provides OpenGL 3.3 and use a session with access to the GPU. |
| The build cannot find GCC or GLFW | Install matching MSYS2 packages for one toolchain. The build guide explains selecting a compiler with `BACKROOMS_CXX`. |
| PowerShell blocks `build.ps1` | Use the included `Build.cmd` or `Run.cmd` launcher, which applies its execution policy only to the launched PowerShell process. |
| Performance is poor or the PC gets hot | Set a 30/60 FPS limit, reduce the window size or turn off Contact Shading, then inspect F1 or a performance report. |
| Music is silent | Check master volume, M mute, Music on/off, music volume and `assets/audio/music/`. Soundtrack errors are written to the log. |

When reporting a problem, include the commit, Windows version, CPU/GPU if known, resolution, level/seed, reproduction steps and `backrooms.log`. Avoid adding personal `settings.cfg`, build outputs or generated captures to commits.

## Further reading and credits

- [Code structure](docs/code-structure.md), [build and tests](docs/build-and-tests.md), [gameplay tuning](docs/balance.md) and [performance diagnostics](docs/performance.md).
- [Level 0 exploration](docs/level0-exploration.md), [Level Fun](docs/level-fun.md), [Parking](docs/parking-upgrade.md) and [the city ending](docs/ending-elevator.md).
- [Course requirements](docs/requirements.md), [report draft](docs/report-draft.md), [demo draft](docs/demo-script.md) and [learning guide](docs/learning-guide.md). Course drafts include earlier project stages and need review before submission.

This educational project was developed with Codex assistance; see [AI-use disclosure](docs/AI_USE.md). The supplied Aranea mesh and recordings are credited separately from project code, procedural geometry and added animation. Sources and available license information are documented in [Third-party credits and licenses](THIRD_PARTY.md) and the individual asset folders.
