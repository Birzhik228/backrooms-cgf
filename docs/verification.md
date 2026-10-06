# Version 0.12 verification

Checked on 6 October 2026 for the Smiler update. The supplied FBX replaces the previous procedural creature. Walk/run animations, closer appearances and a farther exit are implemented.

## Asset and animation

The ZIP contained `source/Zombie Scream.fbx`, 872,240 bytes. Its SHA-256 is `D8E9F3AA2F150B605E13D8CFF5F1E0D16BCE6F6C14C5B7E29EAEA0B29A1DACCF`; the retained source matches it. The original 4,466 vertices, 8,728 triangles, 65-bone rig and black/white materials were preserved. There are no supplied image textures. Blender 4.3 was used offline with automatic script execution disabled; no new runtime library was added.

Idle, walk, run and their three crouched variants were authored on the supplied skeleton. The supplied scream action provides a seventh capture clip. Skinning is baked into 148 frames; the game interpolates compact cached frames and blends gait by actual speed, including fast search behavior. Continuous distance-based phase avoids a reset on a state change. Capture eyes are stabilized around `(0,2.60,0)` to keep the face visible. The 8,071,492-byte cache has SHA-256 `0F247A16F878DB18FC407A965210809EC687DFB02F86D3413AB3B7AEFEB8AF35`. It is validated and loaded before gameplay.

The model suite verifies retained geometry, finite normalized normals, distinct articulated walk/run motion, longer running stride, loop continuity, speed crossfades, deterministic paused poses, identical gait selection during Search/Chase, source capture movement, and full/half stoop bounds of 2.234/2.526 m. Missing, corrupt, truncated, oversized, malformed-index/normal and trailing-data assets are rejected. Source and bake details are in [model notes](../assets/models/smiler/README.md).

## Build and behavior

The final application was compiled using `build.ps1 -Test`, C++17, MinGW GCC `-O2 -Wall -Wextra`. All eight suites passed: world, streaming, movement/settings, threat, imported model, encounter, audio and native music. The native Windows music test ran with normal MCI device access. CMake now copies the model assets and runs its model test from the source root, but this verification used the PowerShell build path.

Threat spawn attempts now use a shared 16–22 m range instead of 25–34 m. Eight-second grace, concealed placement, connected paths, sight/hearing pursuit and five-second memory remain. Tests cover the new range across varied seeds. The graphics check observed a natural spawn at 20.98 m.

The exit is generated at `(6.225,-267)`, with approach `(6.225,-265.8)`, about 270 m from the starting room. Shared cell constants drive its geometry, clear route and interaction. World/streaming tests cover the full northward journey over 260 m, cached streaming without misses, doorway collision/persistence, and absence of the old exit near the start. The rendered traversal ended at `(6.22,-267.53)` with one door interaction and a successful escape; continued exploration also passed.

## Rendered verification

Twenty-five application checks passed with the final asset on Intel UHD Graphics, OpenGL 3.3.0: menu, light/dark Smiler views, low-doorway stoop, opening-door pursuit, two walk and two run poses, standing/crouched/late jumpscares, chase and pause, actual pursuit capture, game over, keyboard and button restart, Escape protection after defeat, settings, distant exit traversal/continue, natural spawn, chase profiling and extended streaming. All reported the imported 8,728-triangle model, six recorded footstep samples, loaded music and zero GL errors. [Commands and logs](v12-render-results.json).

Framebuffer captures were inspected for the supplied mesh's grin/claws, gait changes, stooping, capture framing and menus. Input checks use latched key/button callbacks and the same hit-test path as gameplay. They are automated checks, not a physical mouse/keyboard playtest or headphone listening session. The reproducible graphics script is `tools/verify.ps1`; raw evidence is local under `tests/artifacts/v12`.

## Measured performance and limits

For the 600-frame chase profile, steady median was 2.91 ms, p95 4.51 ms, maximum 6.08 ms. A 3,000-frame generation-only run moved 150.13 m with the threat disabled, 25 active plus 24 prepared chunks, 15 cache promotions and zero synchronous generation fallbacks; median 3.69 ms, p95 11.60 ms, maximum 44.57 ms. These hidden 1280x800 single runs exclude startup and final screenshot readback; desktop activity and timing variation prevent a universal FPS claim. [Measured data](v12-performance-results.json).

The body uses the existing swept footprint collider; extended animated claws are visual geometry. Lighting has no shadow maps and audio uses stereo positioning. The source model's author/license were not provided. Student review, official report/presentation completion and TA-machine testing remain pending. No remote push or course submission was made for this update.

The installed Desktop copy passed a capture-and-restart smoke check launched from an unrelated working directory. All assets loaded and GL errors remained zero. Deployment checked 95 baseline hashes, updated 35 files, preserved settings, and retired the two obsolete model exports after creating `backups/backrooms-v0.11-before-v0.12-20261006-122121.zip`.

---

# Historical version 0.11 verification

Checked on 6 October 2026 for the Watcher update to Threshold. The latest request adds one hostile 3D creature and supersedes the earlier no-enemy preference. The existing seeded rooms, heavy movement, dim lighting, reduced blur, music switch and exit remain.

## Build and behavior

The application was built using PowerShell/MinGW GCC with C++17, `-O2 -Wall -Wextra`. All eight suites passed: world, streaming, movement/settings, threat AI, creature mesh, encounter lifecycle, audio, and native music. The Windows music suite requires normal MCI device access: an initial sandbox run returned Windows error 277, then the complete final build/test run passed outside the sandbox. No new runtime library was added. CMake and Makefile sources/test targets were kept consistent but those build paths were not executed.

Threat checks exercise the sight cone, clear/blocked/animated-door vision, movement hearing, quieter crouching, explicit door noise, bounded A* detours, eight-second grace, five-second memory, capture/reset, pause, deterministic reachable spawning over five seeds and safe recycling during endless travel. Searches expand at most 4,096 nodes. Doors yield to both player and creature, and room shifts protect the creature's chunk and adjacent chunks. A 640,820-query comparison over five door angles produced identical collision results before and after the local-door broadphase optimization.

The original procedural model has 1,740 triangles and 11 articulated parts. Geometry checks cover 450 animated poses, winding, finite unit normals and floor/ceiling bounds. Full doorway stoop remains at most 2.241 m high; half stoop remains below 2.768 m. Clearance uses cached footprints from actual overhead geometry, including 2.40 m doorframes. The editable OBJ/MTL is a neutral export; the runtime uses the same model builder.

Encounter tests verify one-shot capture, finite timing, focus suspension, the 1.05-second scare, locked game over and reset. New cached positional notice/footstep sounds and a centered capture sting pass numerical headroom, priority, master-volume and mixer tests. Physical speaker/headphone listening was not performed.

## Rendered application checks

Twenty final-build checks passed on Intel UHD Graphics, OpenGL 3.3.0, driver 32.0.101.5768: main menu, lit/dark model views, low-doorframe stoop, pursuit through an opening door, chase, pause during chase, standing/crouched/late jumpscare views, game over, actual pursuit capture, keyboard restart, a 960x640 restart-button hit test, Escape during/after capture, settings, physical exit traversal, natural spawn, pursuit profiling and extended streaming. All loaded six footstep recordings and the soundtrack, exited successfully, and reported zero GL errors. [Exact commands and logs](v11-render-results.json).

Captures were inspected for silhouette, hollow eyes, stooping, near-camera framing and readable menus. Restart rebuilds the same seeded world, closes doors and resets threat, movement and stamina. Escape cannot resume a dead run. Tests use the normal latched key/button callbacks; the hidden-window click supplies the real menu hit test with a framebuffer coordinate because an unfocused hidden window cannot warp the OS pointer. This is automated verification, not a physical mouse playtest.

## Performance and limits

The 600-frame pursuit profile stayed in CHASING without capture while the player moved 34.18 m. Steady median was 2.94 ms, p95 4.22 ms, maximum 8.10 ms. A 3,000-frame streamed run with the threat disabled to isolate generation moved 150.13 m, held 25 active plus 24 prepared chunks, promoted 15 cached chunks and had zero synchronous fallbacks. Its steady median was 2.96 ms, p95 4.32 ms, maximum 13.63 ms. Measurements exclude the first 30 and final screenshot frame, use hidden 1280x800 with vsync off and actual wall-clock time. They are individual runs, not general FPS promises. [Details](v11-performance-results.json); raw captures/logs/CSVs are local under `tests/artifacts/v11`.

In a separate release CPU microbenchmark, 24 sealed-route queries each exhausting 4,096 nodes improved from 373.10 ms to 15.70 ms median after limiting door collision checks to nearby owner chunks. Difficult unreachable searches can still cause a short frame spike. Lighting has no shadow maps; positional audio uses stereo panning/distance, with simplified perception muffling rather than acoustic simulation. The low-poly model is an original interpretation of the supplied reference, not the Roblox game's original asset. Student review, official report template, final presentation and TA-machine checks remain pending. No submission or remote push was made for this update.

The installed Desktop copy also passed a capture-and-restart smoke check launched from an unrelated working directory. Deployment checked 82 baseline hashes and updated 36 files while preserving settings. The previous version is backed up as `backups/backrooms-v0.10-before-v0.11-20261006-114719.zip`.

---

# Historical version 0.10 verification

Checked on 5 October 2026 for Threshold. This update enlarges rooms by 25%, puts the exit in a real wall opening, adds hinged doors and the supplied soundtrack, and reduces the lens radius from 1.7 to 0.8 pixels at the reference resolution. No enemies were added.

## Build and behavior

PowerShell/MinGW `build.ps1 -Test` compiled the application with C++17, `-O2 -Wall -Wextra`. All five suites passed: world, streaming, movement/settings, environmental audio and native music. A subsequent interaction-occlusion correction passed the world suite again, and the final application was rebuilt before the graphics checks. CMake/Makefile were updated consistently but were not executed.

World checks cover five seeds, a cut wall aperture and enclosed exit vestibule, the reserved connecting route, intermediate hinge poses, animated collision, blocked-close reversal, pause, cached doors and 128 retained overrides. Interaction rays ignore only the selected door leaf; other leaves and walls still occlude interaction. Settings tests preserve independent music volume and the on/off switch, including old configuration files.

The silent native Windows music suite checks Unicode paths, missing/corrupt input, pause/resume without rewinding, mute, looping and device release. The user's original M4A was copied unchanged; its codec was unsupported by the installed MCI driver, so an offline MP3 conversion is used. The MP3 opens with a duration of 199734 ms. Source and conversion details are in [music provenance](../assets/audio/music/README.md). No additional runtime library or download is required. Physical headphone/speaker listening remains unverified.

## Rendered checks

Sixteen final-build application checks passed on Intel UHD Graphics, OpenGL 3.3.0, driver 32.0.101.5768: reduced-blur starting view, minimum-size settings, music off/save/reload/on, closed/opening/open/closing/reclosed exit, physical exit traversal, continued exploration, a normal moving door, normal-door traversal, a larger hall and a distant negative rendering origin. Every case loaded the soundtrack and six footstep recordings, exited successfully and reported zero GL errors. [Commands and summaries](v10-render-results.json).

Opening the exit alone does not end exploration. Walking through it produces the ending screen. Continuing to the back of the vestibule does not trigger it again. Captures were inspected for door geometry, hinge movement, readable settings and reduced camera blur; keyboard checks use the application's actual input callbacks. Images and logs are retained under `tests/artifacts/v10/`.

Wallpaper phase was separately compared across 405000 rebasing cases, including positive/negative distant origins: maximum UV drift below 0.000004. The 45-metre chunk size keeps world-space texture phase stable through a 180-metre common period.

## Streaming check and limits

A final 2400-frame hidden 1280x800 walk with stamina-limited sprint travelled 121.37 m. Streaming stayed at 25 active plus 24 prepared chunks, with 15 cached promotions and zero synchronous fallback chunks. Excluding the first 30 and final frames, median was 4.16 ms, 95th percentile 6.44 ms and maximum 15.33 ms. [Measured summary](v10-performance-results.json); raw CSV/log under `tests/artifacts/v10/`. This single run is not a general FPS guarantee. Screenshot checks include expensive readback and are not benchmarks.

Dynamic door panels use a separate GPU mesh; static room geometry is not rebuilt during a swing. The game still uses approximate stereo positional sound and lighting without shadow-map occlusion. Settings persist; exploration progress does not. Student review, the official report template, the final presentation update and TA-machine testing remain pending. Nothing was submitted or published.

---

# Historical version 0.9 verification

Checked on 5 October 2026 for Quiet Signal. The user's updated room-variety request supersedes the earlier single-room preference; the user explicitly retained no enemies and environmental tension. Older verification below is historical.

## Build and CPU checks

PowerShell `build.ps1 -Test`, MSYS2 MinGW64 GCC, C++17, `-O2 -Wall -Wextra`, Windows x64. Main application compiled successfully and all four suites passed:

- World: seeded connectivity, six room types, real dead ends, pillars, surfaces, lamp states/phases, mesh winding, exit access, door collision/open/close, safe closing and bounded 128-door memory.
- Streaming: 25 active plus 24 prepared chunks, cache reuse, deterministic lamp/door promotion, door revisions, teleport/new-seed handling and no generation misses during tested paths.
- Movement/settings: acceleration/braking, frame-rate-independent stamina/recovery, pause/reset, mouse smoothing, settings save/reload and malformed values.
- Audio: surface footsteps, bounded WAV parsing, cadence, 3D distance/stereo panning, seeded ambience, tension stingers, headroom, volume and pause/reset.

A standalone real OpenGL check compiled both shader programs and inspected all four generated normal maps: non-flat forward-facing normals, decoded unit-length error below 0.0055, and zero GL errors. CMake/Makefile were kept consistent, but the executed build was PowerShell/MinGW; other toolchains remain unverified.

## Application and visual checks

Intel UHD Graphics, OpenGL 3.3.0, driver 32.0.101.5768. Twenty-one application cases passed: minimum-size main/settings screens, settings navigation/save, pause, normal camera, open/pillar/dead-end/tiled/damp rooms, dying/off fixtures, closed/open/reclosed/traversed doors, stamina exhaustion/recovery, crouch, exit and distant coordinates. All exited successfully, loaded six recordings and reported zero GL errors. Exact commands and summaries: [v9-render-results.json](v9-render-results.json).

Stamina reached zero during sprinting and forced walking at 2.5 m/s, then returned to 100 after recovery. A door test crossed from x=1 to x=-5.28 through the opened panel. Saved settings reached volume 0.80. Captures were inspected for readable menus/HUD, intact geometry, dark fixtures, floor differences and door prompts.

A subsequent menu correction resets selection to Resume/Continue on fresh pause and exit screens. Four final-build regressions passed: settings/resume, opening settings then reaching the exit and pressing Enter to continue, pause, and settings at 960x640. They also confirm the percent glyph. [Final results](v9-final-results.json). Images/logs are in `tests/artifacts/v9/`.

These are automated callback/keyboard checks and framebuffer inspections, not a physical mouse/keyboard or headphone listening session. Mouse handling was reviewed for matching row coordinates and window-to-framebuffer scaling.

## Bounded streaming and frame times

Two sequential final-build profiles used hidden 1280x800 windows, vsync off and seed 12071998, with no concurrent graphics validation. Steady statistics exclude first 30 and final frame. No screenshots were taken during these profiles. Automated movement advances at 1/60 second per frame; timings use actual wall time.

| Scenario | Frames | Median | 95th percentile | Maximum steady frame |
| --- | ---: | ---: | ---: | ---: |
| Stationary | 1200 | 5.86 ms | 7.18 ms | 14.77 ms |
| Forward movement with stamina-limited sprint | 3600 | 4.77 ms | 6.61 ms | 27.38 ms |

The movement run travelled about 178.7 m, promoted 25 cached chunks and reported zero synchronous generation fallbacks, at most 25 active and 24 prepared chunks, and no steady frame above 100 ms. These individual hidden runs do not prove a general speed improvement or guarantee visible frame rate. Startup asset/mesh loading is outside steady measurements; functional captures include screenshot readback and are not benchmarks. Initial loading and teleports can pause. [Summary](v9-performance-results.json); raw CSVs/logs in `tests/artifacts/v9/`.

## Audio and scope

Existing WinMM is reused; no library installation or runtime download was added. Six credited recordings remain intact. The cached stereo mixer uses at most 12 one-shot voices, with world-space sources/listener positions. The new `assets/movement-sounds.wav` is a 12-second, 22050 Hz stereo 16-bit preview of three surfaces, positioned creaks/drips, a door and tension. Peak was 7558/32767 without clipping. It was checked numerically; physical speaker/headphone playback remains unverified.

Normal maps affect shading only. Lighting has no shadow-map wall occlusion; spatial sound uses stereo panning/distance rather than HRTF or acoustic propagation. Doors slide open/closed without a swing animation. Settings persist; gameplay progress does not. The latest 128 open-door overrides survive chunk unloading within a session. Defaults retain dim lighting and reduced blur.

README, requirements, report draft, learning guide, demonstration script and AI disclosure describe v0.9. The old presentation and v0.8 result files are historical. Student review and TA-machine testing remain pending. The Desktop copy receives a separate smoke check during delivery. Nothing was submitted or published.

---

# Historical version 0.8 checks

## 3 October 2026: reduced blur and darker lighting

This shader-only adjustment supersedes the stronger-focus appearance described in the original v0.8 checks below. The executable, room generation, movement and recorded audio are unchanged.

The lens radius is now 1.7 + 0.7 * edge pixels at the 800-pixel reference height, down from 4.6 + 2.0 * edge. Ambient fill is approximately 85% lower and fluorescent illumination approximately 78% lower in linear light. Lamp-face emission and distant fog are dimmer. The flashlight remains useful, while exit-sign emission and clue visibility are preserved. These percentages describe shader parameters, not perceived screen brightness.

Four targeted 1280x800, 60-frame hidden runs passed on Intel UHD Graphics: wide view with flashlight off, the same view with flashlight on, normal HUD at the spawn, and exit approach with flashlight off. Each exited successfully, loaded six recorded footstep samples, and reported zero OpenGL errors. Before/after captures were visually checked for reduced softness, darker rooms, visible wall wear and readable UI/exit sign. Evidence is in tests/artifacts/lighting-tune/. No new CPU suite or performance benchmark was needed for these shader constants.

---
# Version 0.8 verification record

Date: 3 October 2026. Codex implemented and checked this version; AI_USE.md describes the assistance.

## Delivered changes

- Stronger soft focus: a weighted 13-sample lens pass with a 4.6-pixel centre radius at 1280×800, increasing toward the edges. v0.7 used five samples and a 1.15-pixel radius. The HUD is drawn after the lens and stays sharp.
- Worn yellow wallpaper: eleven irregular groups of scratches and torn-paper marks across a 12×3.2 m sheet, plus dark paper joins, low scuffs and damp streaks. A 1536×512 RGB texture is baked at startup. Wear is material colour, not physical holes or displaced geometry.
- Six recorded shoe footfalls replace synthetic step tones during normal operation. The source is GboxMikeFozzy's [CC0 Footsteps collection](https://opengameart.org/content/footsteps-0), recorded while walking through a subway. The game filters the clips for softer carpet playback and varies gait volume and pitch. Hum and clothing rustle remain procedural.

The world stays empty, with one classic yellow room style. Speeds remain 2.5/4.0/1.1 m/s, FOV 78/82, with the same acceleration, braking, exit, clues and infinite connected streaming.

## Build and four CPU suites: passed

Windows, MSYS2 MinGW64 GCC 16.2.0, C++17, GLFW/GLAD and OpenGL 3.3 core. The final build used build.ps1 -Test with -O2 -Wall -Wextra and no warnings. World, streaming, movement and audio suites passed.

The expanded audio checks cover six distinct cached recordings, repeatability, quiet crouch/heavier sprint, peak headroom, smooth sample edges, frame-rate-independent cadence, silent stationary/wall/paused movement, teleport rejection and offline WAV output. RIFF validation covers bounded chunk sizes, unknown/odd chunks, truncation, incorrect PCM format/rate/channel count and missing-bank fallback/reload. No runtime Python, decoder package or network access is required.

CMake now copies assets/audio beside its executable and runs the audio tests from the source directory. The executed build was PowerShell/MinGW; other toolchains remain unverified.

## Graphics: 21 checks passed

Hidden application runs on Intel UHD Graphics, OpenGL 3.3.0 Build 32.0.101.5768 all exited with zero GL errors, loaded all six recordings and reported YELLOW HALLS.

The suite covers the normal view, all former district locations, minimal camera HUD, close wall wear in two chunks, 1600×900 soft focus, slow walk/crouch/faster movement, standing, braking, wall collision, map/clue, exit/continuation, 960×640 menu, distant coordinates and extended walking. Both gameplay and streaming summaries are saved in [render-test-results.json](render-test-results.json).

Visual review of close and wide captures confirmed noticeably stronger blur, legible HUD, visible scratches/peeling/damp marks and intact room geometry. Wallpaper mapping repeats every 12 m with a 36 m wrapping phase, matching the rendering-origin step; damage does not depend on player time or camera motion. No additional geometry or per-frame wallpaper texture samples are used.

The complete graphics suite preceded the final audio-only rebuild. The final executable then passed all CPU suites, the 1,200-frame stationary profile, 3,600-frame streaming profile and recorded-audio preview export. Installed-copy checks follow below. Captures are not a physical keyboard/mouse/audio playtest.

## Performance scope

The larger wallpaper adds approximately 2 MiB including mipmaps. The stronger lens uses 13 samples instead of 5, retaining one fullscreen pass and the existing framebuffer. Existing chunk prefetch/reuse and two-frame GPU queue limits remain.

One v0.7/v0.8 comparison used 1,200 stationary frames each, hidden 1280×800, vsync off, seed 12071998, position(3,3), yaw−90 and pitch−3. World, camera and movement are identical; wallpaper and lens differ. No other game or graphics validation ran concurrently.

| Run | Median frame | 95th percentile | Maximum steady frame |
| --- | ---: | ---: | ---: |
| v0.7 reference | 6.98 ms | 10.08 ms | 14.32 ms |
| v0.8 stationary | 6.53 ms | 9.70 ms | 13.29 ms |
| v0.8 streaming sprint | 3.88 ms | 6.61 ms | 54.35 ms |

These are individual runs, not proof of a general speed improvement. All had zero steady frames above 100 ms. The 3,600-step sprint travelled 239 m, crossed six chunk boundaries and promoted 30 prepared chunks with zero synchronous fallback generation.

Steady statistics exclude first 30 and final frame; initial process/asset/world loading is outside frame timings. Functional captures include initial frames and screenshot readback and may report larger maxima. Visible performance varies with hardware, routes and system load; initial loading/teleports may pause. Raw logs/CSVs are in tests/artifacts/performance, with phase statistics and exact method in [performance-results.json](performance-results.json).

## Recorded assets and preview

The six PCM WAV files are 22050 Hz mono 16-bit. All six SHA-256 hashes match assets/audio/footsteps/manifest.json. Original OGG downloads, source URLs and processing details are retained there. Credit/license details are in THIRD_PARTY.md and docs/licenses/footsteps-CC0.txt.

The refreshed 10-second assets/movement-sounds.wav demonstrates walking, crouching and faster movement on carpet, followed by ambience. Export logged six loaded recordings; its peak is 13850/32767 with no clipping. Actual speaker/headphone playback has not been verified by listening. Startup now logs whether an output device opened; M retries/toggles sound. Missing WAVs are logged and use a procedural fallback, but all delivered verification used the recordings.

README, report draft, requirements, learning guide, demo, AI-use note and deck reflect these changes. All 10 slides passed 1600×900 fit/navigation checks; changed slides were visually inspected.

## Handoff and remaining limits

All 73 baseline file hashes were checked before replacement. The previous runnable version is saved in backups/backrooms-v0.7-before-v0.8-20261003-173818.zip. Deployment copied and hash-verified 92 files and removed 19 backed-up obsolete previews/profile files. The installed copy launched from an unrelated directory, loaded all six recordings and completed 120 walking steps to (3,-1.61) with zero GL errors. Evidence: tests/artifacts/desktop-handoff-v8.log and its BMP; the 144.27 ms full-run maximum includes initial frames and screenshot readback.

The previously running v0.7 game was closed gracefully for replacement. The installed v0.8 game was reopened at its normal start menu; its window title was verified and startup logged Audio output active. Actual speaker/headphone listening remains unverified. The final record was copied separately and hash-verified.

Lighting has no wall-occlusion shadows; wallpaper motifs repeat and tests sample the procedural domain. Student review, course template/team/deadline confirmation and TA-machine testing remain pending. Nothing was submitted or published.


## Version 0.9 Desktop delivery

The existing game was not running. The previous source/assets/executable were backed up to backups/backrooms-v0.8-before-v0.9-20261005-200949.zip. All 92 baseline file hashes matched before replacing anything, and 31 updated/new files were copied and hash-verified. The installed executable was launched hidden from an unrelated working directory, opened the nearby optional door and walked through it from (1,9) to (-3.61,9) over 120 frames. It loaded all six recordings and reported zero GL errors. Evidence: tests/artifacts/v9/installed.log and installed.bmp. This launch is an installation check, not a frame-time benchmark. No visible game was opened.
