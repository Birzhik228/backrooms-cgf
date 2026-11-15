# Ambient occlusion performance at 1280 x 720

Historical measurement: this report identifies the 14 November binary and
preceding asset set. The 15 November submission replaces the creature and some
audio; use that package's own benchmark results for current-build claims.

Measured locally on 14 November 2026 using the current Windows Release build.
The median of three trial averages was **122.24 FPS with AO ON** and
**142.80 FPS with AO OFF**. Disabling AO increased this median by **16.81%**.
Every trial averaged above the course's 30 FPS target in this scene. This is a
hidden, frozen presentation-scene benchmark, not a guarantee for all gameplay
or evidence from the lab computer.

## Summary

The FPS and timing summaries below are medians across three trials per setting;
the range gives the lowest and highest trial-average FPS.

| Setting | Median average FPS | Trial-average FPS range | Mean frame time | p95 frame time | p99 frame time |
| --- | ---: | ---: | ---: | ---: | ---: |
| AO ON, intensity 0.85 | 122.24 | 119.68-127.08 | 8.18 ms | 10.59 ms | 13.59 ms |
| AO OFF, intensity 0 | 142.80 | 140.47-146.99 | 7.00 ms | 8.95 ms | 10.77 ms |

AO ON measured 11,072 frames over 90.016 seconds; AO OFF measured 12,909 frames
over 90.011 seconds. One AO-on frame and two AO-off frames exceeded the 33.333 ms
budget for 30 FPS. The worst measured frames were 54.67 ms with AO ON and
52.81 ms with AO OFF. Thus the average and percentile results exceed the target,
but the measurements do not establish an uninterrupted 30 FPS minimum.

## Individual trials, in execution order

| Trial | Average FPS | Mean ms | p95 ms | p99 ms | Maximum ms | Frames over 33.333 ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| AO ON 1 | 122.24 | 8.18 | 10.59 | 14.47 | 28.64 | 0 |
| AO OFF 1 | 146.99 | 6.80 | 8.17 | 9.90 | 52.81 | 2 |
| AO OFF 2 | 142.80 | 7.00 | 8.95 | 10.77 | 21.75 | 0 |
| AO ON 2 | 119.68 | 8.36 | 11.27 | 13.59 | 54.67 | 1 |
| AO ON 3 | 127.08 | 7.87 | 9.74 | 11.54 | 22.28 | 0 |
| AO OFF 3 | 140.47 | 7.12 | 9.26 | 10.79 | 20.59 | 0 |

## Computer and build

- Computer: THUNDEROBOT 911S.
- CPU: Intel Core i5-12450H, 8 cores / 12 logical processors.
- RAM reported by Windows: 15.73 GiB.
- Actual OpenGL renderer in all six logs: **Intel(R) UHD Graphics**.
- OpenGL: `3.3.0 - Build 32.0.101.5768`; Intel driver: `32.0.101.5768`.
- An NVIDIA GeForce RTX 3050 Laptop GPU is also installed; these results use
  the Intel renderer, not the NVIDIA adapter.
- OS: Windows 11 Home, version `10.0.26200`, build `26200`.
- Power scheme: Balanced; AC power confirmed in the post-run check.
- CMake configuration: Release, GCC, `-O3 -DNDEBUG`.
- Executable SHA-256:
  `0F6FCEE1A60E56034183A331BF41274B9CBA11101355AFEF980A6BBA0D674DCE`.

The JSON records the source commit and modified/untracked file list. The
executable hash identifies the measured binary, since the working tree contains
the recent presentation, controls and console changes.

## Method

1. Use the fixed Level 0 presentation entrance: seed 12071998, position
   x=3.75/z=3.75, yaw -90 degrees, pitch -5 degrees, FOV 78 degrees, exposure
   1.1 and blur 0. Gameplay, actor poses, shader animation and head bob remain
   frozen. The normal live rendering passes and team/HUD overlays remain active.
2. Request 1280 x 720, run hidden with VSync disabled and no FPS cap, and keep
   the normal bounded GPU queue. Hidden mode avoids the unfocused-window 30 FPS
   cap and suppresses audio output. Music is disabled in a dedicated fixture.
3. Enable presentation AO at 0.85 for ON. For OFF, the existing presentation
   check injects the actual G control after the first three rendered frames.
   Its assertion verifies that the AO pass is skipped; the toggle occurs during
   the excluded warmup.
4. Run trials sequentially in ON/OFF, OFF/ON, ON/OFF order. No concurrent game,
   build or graphics test was launched. Ordinary desktop applications remained
   open, so this is a local desktop measurement rather than an isolated lab run.
5. Discard whole frames until their cumulative recorded time reaches five
   seconds. Measure subsequent whole frames until they total at least 30 seconds.
   Keep the small final-frame overshoot. Each process has a 40-second runtime
   limit to allow the full measurement window; unused tail frames are excluded.
6. Compute `average FPS = 1000 / mean(frame_ms)`. Compute p95/p99 using nearest
   rank on the measured CSV samples. The headline comparison is the median of
   each setting's three average-FPS values. Original JSON performance reports
   include warmup and tail frames, so their aggregates differ from this report.

The recorded frame time includes CPU rendering work, bounded GPU queue waits
and buffer swapping. It excludes small post-frame telemetry bookkeeping and
is not an isolated GPU timer. Initialization, captures and shutdown are not
included in the selected measurement windows. The observed FPS difference is
an end-to-end frame comparison, not a direct measurement of the AO shader alone.

## Verification and retained evidence

All six measured runs exited successfully with empty stderr, zero OpenGL
errors and zero retained application-owned GL objects. Logs confirm one AO
pass per frame for ON and only the first three warmup passes for OFF. The
script verified the same camera direction, FOV and drawn geometry in every
measured window, with no mesh uploads. Normal player settings and the benchmark
fixture were unchanged.

Two separate, untimed capture runs verified actual **1280 x 720** BMP dimensions
for both settings. Their central scene regions differed at 6,915 sampled pixels,
confirming that the effect visibly changed the scene outside the HUD.

- [Machine-readable results, settings, hardware and exact arguments](ao-performance-2026-11-14.json).
- [Raw evidence directory](../tests/artifacts/ao-performance/20261114-182159-882/):
  six frame CSV files, original performance JSON files, stdout/stderr and copied
  logs, the settings fixture, metadata, results and two BMP captures. This
  timestamped directory is local and excluded from Git by the existing rules.
- [Repeatable benchmark script](../tools/benchmark-ao.ps1).

From the project root, after building and closing other game instances:

```powershell
.\tools\benchmark-ao.ps1
```

For the final course submission, repeat the benchmark on the lab computer and
check a representative moving gameplay route. Use these figures as measured
evidence for the fixed demonstration view on the named computer.
