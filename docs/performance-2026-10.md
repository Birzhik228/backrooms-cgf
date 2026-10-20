# Performance and navigation checks — 20 October 2026

The reported heating on a friend's computer could not be reproduced on that
computer: hardware, temperatures and FPS were unavailable. These measurements
are local observations on Intel UHD Graphics, not a thermal guarantee.

## Findings and changes

- Level transitions already replace the World and release old chunk GPU meshes.
  Actual Level 0 → Fun and Fun → Parking runs each released 49 chunk meshes and
  left zero old chunk GPU entries. Streaming tests maintain the 25 active +
  24 prepared chunk bound. No stacking of previous levels was found.
- Normal play had VSync but no independent frame limit. Parking can render faster
  than Level 0, increasing frames per second and hardware work when VSync is off
  or the display refresh rate is high. Normal play now defaults to 60 FPS, with
  30 FPS in menus and unfocused windows. Settings offer 30/60/90/120/144/240 and
  unlimited. Existing preferences are preserved.
- Frame pacing blocks on a high-resolution Windows waitable timer; it does not
  spin or change the global timer resolution. Other systems and timer-creation
  failure fall back to a standard thread sleep. Slow frames never trigger
  catch-up render bursts. Hidden regression runs remain uncapped by default;
  `--fps-limit 60` enables pacing explicitly.
- Point-light shadow maps now retain slots by global lamp identity. Selecting a
  different nearest-light set or rebasing the render origin no longer rebuilds
  maps for unrelated retained lamps. Door and geometry changes still invalidate
  affected maps. Shadow resolution, light count, textures and geometry are unchanged.
- Creature navigation approaches an accessible point beside a wall-hugging player
  and skips reachable grid attachment points that previously pulled it backward
  at corners. Swept collision, forward turning and periodic route refresh remain;
  newly opened doors can shorten a detour.

## Measurements

Seed 42, 1280×800, 480 frames, no threat, deterministic simulation. Frame means
exclude the first 60 warm-up frames. Uncapped results vary with system load.

| Parking walk | Before | After |
|---|---:|---:|
| Lamp shadow map updates | 60 | 12 |
| Mean frame time, ms | 6.228 | 6.101 |
| Mean drawn triangles | 16,553.895 | 16,553.895 |

The reduction in shadow updates is 80%; the modest frame-time difference should
not be treated as a general FPS improvement. The matched final parking image was
pixel-identical before and after.

For the updated executable in the stationary parking fixture:

| Mode | Mean frame time | Process CPU time / wall time |
|---|---:|---:|
| Uncapped | 5.183 ms | 1.719 s / 3.626 s |
| 60 FPS cap | 17.083 ms (~58.5 FPS) | 2.188 s / 9.719 s |

CPU time divided by wall time fell from 0.474 to 0.225 CPU-seconds per second
(about 52%). These process measurements include startup and shutdown. They are
not whole-machine CPU percentages and do not measure temperature or power.
The capped run takes longer because it intentionally renders fewer frames per
second. Parking geometry remains larger than Level 0; this change does not
claim to reduce its memory footprint.

## Verification

- Native world, streaming, progression, parking/darkness, Fun, partygoer,
  atmosphere, movement/settings, threat, model, encounter, audio and music suites.
- Shadow-slot retention/eviction/rebasing regression; 28 rendered occlusion probes,
  24 AO fixtures, and lens-noise tests.
- All 14 parking runtime scenarios, both level transitions, and creature
  door-chase/restart checks.
- Wall/corner navigation at 10/30/60 Hz and a newly opening door during pursuit.
- Settings screenshot inspected; frame cap save/load and bounds tested.

Raw CSVs, process measurements, screenshots and logs were retained in the working
copy under `tests/artifacts/performance` and `tests/artifacts/navigation`.
