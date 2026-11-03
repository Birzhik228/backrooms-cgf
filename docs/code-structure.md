# Application structure and level lifecycle

The application controller is separate from world generation, rendering, audio
and creature implementations. The entry point is `backrooms.cpp`; application
modules live in `app/` under `br::app`. Resource ownership, diagnostics and validated
startup tuning share these boundaries. Shipped gameplay defaults, asset formats
and controls remain unchanged; these upgrades add no runtime library.

## Responsibilities

| File or module | Responsibility |
| --- | --- |
| `backrooms.cpp` | Select the asset directory, parse options, validate/freeze balance before constructing gameplay, load sound banks, handle audio export, call `run()` and report startup errors. |
| `app/game.h` | Small public application boundary: `run(const Options&)`. |
| `app/game_internal.h` | Internal `Game` coordinator, its private state and named phase methods. It is not a public interface for `world` or `common` modules. |
| `app/game_state.h` | Plain `PlayerState` and `FrameTimings` data shared by the coordinator's phases; reset policy remains in the session lifecycle. |
| `app/game.cpp` | Keep `WindowContext` alive around `Game`, initialize settings/input/previews, and audit resources after `Game` destruction. |
| `app/gl_context.h` | Own GLFW initialization and the window/context, including partial initialization failures. |
| `common/gl_resource.h` | Move-only buffer, VAO, texture, framebuffer, program, shader and sync owners with acquisition/release counters. |
| `common/balance.h` / `balance.cfg` | Typed gameplay defaults, atomic file validation and read-only configuration for the running session. |
| `app/game_loop.cpp` | Order the frame phases, preserve the existing simulation timestep and frame pacing, write diagnostics, and perform normal shutdown. |
| `app/options.h/.cpp` | Parse and validate all existing command-line flags, including hidden verification modes. |
| `app/input.h/.cpp` | Latch key and mouse-button presses, read held keys, smooth mouse look, and control cursor capture. Callbacks collect input rather than selecting game screens. |
| `app/flow.h` | Represent the single current application screen and the run metadata used by the menu. |
| `app/game_menu.cpp` | Handle menus, settings, the controls guide and level-selection commands. |
| `app/game_play.cpp` | Order gameplay phases, with separate helpers for movement, streaming, exit progression, creatures, capture and animation. Existing algorithms remain in the corresponding `common/` classes. |
| `app/game_interactions.cpp` | Handle key/Verity pickup, breakers, presents, hiding places, observations and doors in the existing order; return pickup/noise events to creature perception. |
| `app/game_audio.cpp` | Feed the audio scene and level sound directors, schedule ambience and footsteps, and update music. |
| `app/game_render.cpp` | Order mesh, camera, light, shadow, scene and lens helpers using a local `RenderFrame` for values shared by one rendered frame. |
| `app/game_hud.cpp` | Draw gameplay information, prompts and application overlays. |
| `app/game_session.cpp` | Centralize screen selection, player reset and world replacement for restarts and level changes. |
| `app/game_ending.cpp` | Run the cinematic timeline, its input/audio handling, replay and return to the menu. |
| `app/game_scenarios.cpp` | Inject scripted input/stress transitions, hold the fixed-timestep predicate and provide fixture hooks at the gameplay phase they prepare or observe. |
| `app/render_resources.h/.cpp` | Mesh upload/release, render targets, frame-queue pacing and screenshot helpers extracted from the former main file. |
| `app/minimap.h/.cpp` | Draw the minimap from world and player information. |
| `app/diagnostics.h/.cpp` | Open the application log, write messages and report GLFW errors. |
| `app/performance.h/.cpp` | Sample process memory/resource counts and write bounded session reports after graphics cleanup. |
| `common/performance.h` | Fixed-size frame-time histograms and a bounded list of worst frames. |

`World` still owns seeded generation, streaming, collision and world interaction
state. Creature behavior, movement, audio, materials and cinematic geometry remain
in `common/`. The application files coordinate those components; they do not each
create their own independent world or audio engine.

The ordinary frame path is input/scenarios, menus, gameplay, audio, world render,
HUD and frame completion. The ending uses its separate cinematic path and keeps
the ordinary gameplay state suspended. Shared values needed across phases belong
to the private `Game` instance rather than a collection of application globals.

## Smaller gameplay and rendering phases

`updateGameplay()` exposes the execution order: door animation/collision, player
movement, world streaming, level-exit handling, interactions, creatures, encounter
state and creature animation. Each helper retains one part of that sequence.
`updateInteractions()` lives in `game_interactions.cpp`; its named helpers keep
object-specific rules readable and return the key-pickup and door-noise events
used by creature perception.

Fixture hooks stay beside the phase they prepare. For example, the original
threat-position snapshot still precedes movement and shifting, while the
perceived-player snapshot precedes later mouse/actor fixtures. Moving all fixture
mutations to frame start would change those observations. Normal and scripted
input retain their existing short-circuit conditions; `usesFixedTimeStep()` names
the existing fixed-step predicate rather than introducing a new time policy.

`PlayerState` groups position, motion, stamina, hiding, crouching, camera height,
head-bob displacement and travel counters. `FrameTimings` groups wall-clock phase
measurements separately from simulation `dt`. They are data structures, not new
subsystems. `resetPlayer()` remains explicit: returning to an entrance and
starting a new level intentionally retain different session values.

`renderWorld()` orders mesh updates, target setup, camera/lights, shadows, scene
geometry/actors and the lens pass. Its local `RenderFrame` carries the shared
origin, camera matrices, eyes, flashlight and selected lights between helpers.
It owns no persistent GPU resources and expires after the frame. Origin
subtraction retains double precision, while camera/light calculations retain
their original float types. The refactor is intended to preserve render-call and
calculation order; visual and movement comparisons are recorded below.

## Screen state and encounter state

`GameFlow` contains one `Screen`: `Gameplay`, `Menu`, `Settings`, `Controls`,
`LevelSelect` or `Ending`. Selecting one screen replaces the previous screen.
`Game::setScreen()` synchronizes that selection with the input pause/cursor mode;
application code should use this method instead of independently changing the
screen and `Input::paused`.

The `entered`, `guidePending` and `escaped` values describe run history. They are
not additional screens. Opening Settings and returning to Menu must retain the
information that a run has started, that the initial guide is pending, or that an
exit was reached. Clearing those values belongs to the relevant resume/restart
operation rather than every screen change.

Capture and defeat remain owned by the existing encounter manager. Screen
selection alone does not cancel a jumpscare or revive the player. Restart resets
the encounter explicitly. Likewise, `endingPaused` controls the cinematic clock;
it does not make the underlying world simulate while the ending screen is active.

## Reset and transition boundaries

| Operation | World state | Player, actors and transient presentation |
| --- | --- | --- |
| `resetPlayer()` | Preserve the current world, collected items, opened doors and powered circuits. Prepare the entrance chunks. | Return to the entrance; reset movement, view, stamina, enemies, capture, animation contacts and transient audio/directors. |
| `freshRun(seed)` | Replace the current level with a fresh world using the requested seed. | Use the shared replacement/reset path, then reset run counters and return to gameplay. |
| `enterLevel(next, selected)` | Replace the world with the target level while keeping the current seed. | Use the same replacement/reset path, then apply checkpoint or explicit-selection flags and notices. |

Both new-run and level-entry operations call `replaceWorld(seed, level)`. That
method installs the new CPU world, releases and clears the old streamed GPU
chunks, releases door/key/mouse meshes, invalidates cached lamp shadows, and
calls `resetPlayer()` once. Actor-specific reset/mesh handling stays in
`resetPlayer()`. This shared order makes the lifecycle visible in one place.

Changing an overlay does not replace the world. Exiting an ending to the menu
clears the cinematic presentation/audio state without implicitly creating a
new level. Clearing a GPU mesh container now releases its move-only GL owners
automatically. Explicit reset methods remain where a level or cinematic must
release resources before the enclosing application ends.

## Ownership, configuration and diagnostics

`WindowContext` encloses `Game` in `run()`. Reverse destruction therefore releases
all game resources before the OpenGL context closes, on both normal exit and
exceptions. GL names passed to rendering calls are borrowed; ownership stays in
`common/gl_resource.h` wrappers. They cannot be copied. `PerformanceMonitor` is
declared before the graphics members so its destructor writes the final resource
snapshot after those members unwind. Counters track application-owned objects,
not driver allocations or GPU memory bytes.

`main()` loads `balance.cfg` (or `--balance-file PATH`) once before constructing
the game. Valid files override named defaults; any parse, range or relationship
error rejects the whole candidate and logs a line diagnostic. The active
configuration stays read-only for the session. Edit the file and restart to tune
movement, stamina and creature perception; rendering geometry and collision
invariants remain fixed. See [balance configuration](balance.md).

F1 reads the monitor's process RAM, GL object count, p95 frame time and slow-frame
total. `--performance-report PATH` enables a shutdown JSON report. Memory is
sampled roughly once per second and at lifecycle events; a 720-sample ring,
16 worst-frame records and fixed histograms bound storage during long sessions.
`tools/verify-performance.ps1` exercises real level changes and an optional
600-second Parking soak. See [performance diagnostics](performance.md) for
commands, checks and the limits of interpreting these measurements.

## Building and extending the application

Existing launch and build commands remain valid. From the project directory:

```powershell
.\build.ps1
.\build.ps1 -Test -GraphicsTest
```

CMake is the single source for dependencies, static libraries, runtime copying
and CTest registration. PowerShell and Make invoke that graph; incremental builds
track application headers and refresh data-only config changes. Native checks
run separately from opt-in OpenGL checks. Keep test entry points under `tests/`,
not `app/`. The [build and test guide](build-and-tests.md) describes presets and
the prepared GitHub Actions workflow; hosted execution requires a push.

For a new screen, add its `Screen` value and route transitions through
`setScreen()`. For state that should reset on a level change or restart, add it to
the shared session lifecycle instead of duplicating reset logic in menu handlers.
Use `balance.cfg` for supported tuning; add new typed keys and validation in
`common/balance.h`. Generation/AI algorithm changes belong in their existing
`common/` or `world` modules.

## Verification — 3 November 2026

### Smaller gameplay/render phases

Compared with checkpoint `47c2d87` (`checkpoint-2026-11-03`), the gameplay
coordinator is 16 lines instead of 441, and the rendering coordinator is 35
instead of 434. Their work now lives in named helpers; these figures describe
coordinator size, not a reduction in total functionality or total source lines.

The Release build passed with GCC C++17, `-O2 -Wall -Wextra`, without warnings.
Seven native suites passed (flow, movement, progression, world, streaming, threat
and encounter), along with all four real-OpenGL checks. All 114 runtime cases
passed: 23 menus, 16 Fun, 14 Parking, 15 ending, 17 Aranea, six forward/scare,
ten progression/hiding and 13 exploration/note/presence cases. Every runtime log
reported zero GL errors and zero owned GL resources after Game destruction.

Eleven before/after comparisons produced identical pixel content and identical
non-timing CSV data: the seven saved checkpoint cases, plus opening a door, a
live chase, a close-up jumpscare and the Level 0-to-Fun transition. This includes
player position, camera values and creature motion where those CSV fields apply.
Assets, shaders, balance files and the existing checkpoint references were not
modified. Comparisons used the same local Windows/Intel UHD environment; this is
not a claim of compatibility with another computer or an FPS improvement.

Evidence is retained in the working folder's
`simplification-validation/candidate-results.json` and `runtime-results.json`.
The build/native logs are in `code-simplification/`; capture commands remain in
`tests/reference/checkpoint-2026-11-03/manifest.json`. The sections below record
earlier upgrades and are separate from these current checks.

### Resource, tuning and diagnostics upgrade

The integrated upgrade passed 28 native and four real-OpenGL CTest cases,
48 full-game scenarios, 27 repeated level changes and a 600-second real-time
Parking soak. Four fresh isolated
before/after comparisons (sprint, pause, creature facing and city) were again
byte-identical, with equal non-timing CSV values. Custom runtime tuning changed
walking speed without recompilation; an invalid mixed configuration restored
all defaults, and a subsequent ordinary launch loaded the shipped configuration.
Every upgraded parity/integration process reported zero owned GL objects after
`Game` destruction. These are behavior/cleanup checks, not an FPS improvement
claim. Measured transition and ten-minute soak results are documented in
[performance diagnostics](performance.md).

### Earlier application split

Release builds passed with GCC C++17, `-O2 -Wall -Wextra`, without warnings.
The initial split included all 15 application translation units in each build
route; the current unified CMake graph also includes the performance module.

Six native suites passed: flow, movement/settings, encounter, progression,
threat navigation and bounded world streaming. Integrated verification passed
91 scenarios:

| Script | Passed scenarios |
| --- | ---: |
| `tools/verify-level-menu.ps1` | 23 |
| `tools/verify-fun.ps1` | 16 |
| `tools/verify-parking.ps1` | 14 |
| `tools/verify-ending.ps1` | 15 |
| `tools/verify-aranea.ps1` | 17 |
| `tools/verify-forward-scare.ps1` | 6 |

The checks exercise keyboard/click menus, modal input consumption, crouch,
level entry, key/circuit reset, checkpoints, capture/restart, the full ending,
cinematic pause/replay/menu return, and the rendered camera during a jumpscare.
All successful runtime cases reported zero OpenGL errors.

Four isolated before/after comparisons (sprinting, pause, creature facing and
city ending) produced byte-identical 960x640 screenshots. Stable game summaries
and non-timing profile columns also matched. These are preservation checks,
not a claim of improved frame rate.

The forward-motion verifier initially expected older walking/running speeds.
The original executable reproduced that failure: current speeds are 1.2 and
3.6 m/s. The verifier now uses those values with a 0.08 m/s tolerance for rounded
CSV coordinates; all six scenarios pass. Gameplay speeds were not changed.

Runtime evidence remains in the corresponding `tests/artifacts/` directories
of the verification copy. Native logs are under
`tests/artifacts/code-structure/native/`. The isolated reference comparisons
are retained separately in the working folder's `code-structure-parity/`.
