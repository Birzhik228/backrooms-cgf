# Performance and resource diagnostics

Press **F1** in gameplay to show process resident RAM, currently owned OpenGL
objects, frame-time p95 and the slow-frame count alongside the existing world
and rendering counters. These are diagnostic values, not an FPS improvement
claim or a hardware requirement.

## Capture a report

```powershell
.\Backrooms.exe --performance-report tests/artifacts/session-performance.json
```

The report is written during shutdown, including exception unwinding after
performance monitoring has started. Its final resource snapshot is taken after
Game's graphics owners have been destroyed and before the OpenGL context closes.
Normal logs also contain `GL resources after Game: 0`.

Useful options:

| Flag | Meaning |
| --- | --- |
| `--performance-report PATH` | Write a JSON report to this path. |
| `--slow-frame-ms 50` | Slow-frame threshold in milliseconds; default 50. |
| `--run-seconds N` | Stop after N seconds of real running time, independent of simulated frames. |
| `--stress-cycles N` | Exercise N complete 0 -> Fun -> Parking -> 0 cycles using real level-entry cleanup. |
| `--stress-interval 120` | Frames between stress transitions; default 120. |
| `--fps-limit 60` | Cap frame production; use 0 for an uncapped diagnostic run. |

Stress mode and scripted movement use a fixed simulation step for reproducible
behavior. Frame timings and `--run-seconds` always use wall time. A fixed number
of hidden frames is therefore not evidence of ten minutes of real gameplay.

## Repeated transitions and a ten-minute soak

Build the current project first, close any other instance of the same executable,
then run the following from the project folder. Verification processes run
sequentially because the executable writes one shared `backrooms.log`.

```powershell
# Shorter transition verification: nine cycles, 27 actual level changes.
.\tools\verify-performance.ps1 -SkipSoak

# Full check: transitions followed by 600 real seconds in Parking.
.\tools\verify-performance.ps1

# A shorter development smoke test; this is not a ten-minute soak.
.\tools\verify-performance.ps1 -Cycles 3 -SoakSeconds 30
```

`-Executable` selects another built executable. `-MemoryGrowthLimitMiB` changes
the default conservative 64 MiB sustained-growth threshold. Each invocation
creates a timestamped folder beneath `tests/artifacts/performance/`, containing
the two reports, copied logs and `validation.json`. A timeout stops only the
process launched for that verification case.

The transition case uses a fixed seed, hidden gameplay and no threats, with the
FPS cap disabled. It verifies the requested transition count, same-level settled
resource counts after the first visit, bounded world caches, and final ownership
balance. The soak renders Parking for the requested real duration at a 60-FPS
cap, using a fixed-position camera tour with threats disabled. It exercises
continuous Level 3 rendering and room shifting without entering a game-over
menu; it does not cover human navigation through the entire map.

The checker requires zero owned GL objects at shutdown, matching acquisition and
release totals, at most 25 resident plus 24 prepared chunks, at most 49 chunk GPU
meshes, and at most two frame fences. GL object counts must plateau for comparable
warmed samples; the transient fence count is excluded from that plateau test.

For memory, comparisons use only the same level after warmup. The first and last
three samples provide baseline and late medians. A failure requires the late
median **and all three late readings** to exceed the baseline by the configured
threshold. A one-time high-water mark is reported rather than treated as a leak:
allocators, the OS and graphics driver can retain memory after release. Fewer
than six warmed samples are explicitly marked as insufficient for a sustained
memory-growth check. A successful short run cannot replace a long soak.

## JSON schema 1

- `frames`, `duration_seconds`: observed frames and monitor wall duration. The
  monitor duration includes initialization/cleanup around the active game loop.
- `frame_ms`: mean, p50, p95, p99 and maximum complete frame time, including swap,
  GPU queue waits and the intentional FPS-cap sleep.
- `work_ms`: the same statistics before the intentional cap sleep; this still
  includes CPU work and waits and is **not** isolated GPU execution time.
- `slow_threshold_ms`, `slow_frames`, `worst_frames`: a frame is slow when it
  exceeds both the threshold and twice its configured frame budget. Up to 16
  worst frames retain frame number, full/work times, budget, level and cinematic
  state. Ordinary capped menu frames are not automatically classified as stalls.
- `peak_resident_bytes`, `peak_sampled_private_bytes`, `private_memory_kind`:
  process memory. Windows reports working-set RAM and private committed bytes;
  Linux reports resident RAM and private resident pages where available.
- `resource_kinds`, `created`, `destroyed`, `final_alive`: ownership counts in
  order buffer, vertex array, texture, framebuffer, program, shader and sync.
  These count application-owned GL objects; they do not measure GPU VRAM bytes
  or hidden driver allocations.
- `peak_observed_gl_objects`: highest count observed by the monitor.
- `samples_total`, `samples_retained`, `samples`: roughly one periodic sample per
  second, plus lifecycle events such as `session-start`, `level-leaving`,
  `level-ready`, `level-settled` and `session-end`. Each sample includes level,
  world/GPU chunk counts, resource counts and memory-availability flags.

Histograms have fixed storage. Percentiles use 0.25 ms bucket bounds; maxima
remain exact. The resource/memory sample ring retains at most 720 records and
worst-frame history at most 16 records, so enabling diagnostics does not grow
memory with play duration. Very long runs retain a recent window of samples;
cumulative frame statistics and resource creation/deletion totals cover the
whole monitored session. Memory peaks between samples may be missed (the Windows
working-set high-water counter also contributes to the reported resident peak).

No fixed FPS assertion is made: resolution, GPU, driver, frame cap and concurrent
work affect timing. Compare equivalent settings and scenes before drawing a
performance conclusion.


## Executed verification — 3 November 2026

The Windows release build passed nine cycles (27 actual level changes, 3,360
frames), followed by 600 real seconds of Parking rendering (34,793 frames;
605.06 seconds including process startup/cleanup). Both runs finished with zero
owned GL objects and matching creation/deletion totals for every resource kind.
After warmup, comparable samples retained identical GL counts; world/GPU caches
stayed within their limits.

Parking private committed memory had a warmed baseline median of 598.66 MiB and
late median of 599.23 MiB: +0.57 MiB. Peak process working set was 605.85 MiB.
The cycle run's same-level private-memory median changes were +1.70 MiB (Level 0),
+5.20 MiB (Fun) and +2.96 MiB (Parking). Its peak working set was 1,353.99 MiB;
levels have different resource demands, so this peak alone is not evidence of a
leak. Neither run crossed the sustained-growth threshold. These finite checks
detect the tested accumulation patterns; they do not prove absence of every leak.

The Parking report captured ten slow frames, p95 18 ms, p99 19.5 ms and maximum
1,743.42 ms. Other validation processes overlapped part of this run, so these
numbers demonstrate reporting rather than a controlled performance comparison.
Synchronous loading/generation can still produce long frames. The fixed-position
soak does not replace navigation coverage or testing on the teammate's hardware.

Reports, copied logs and `validation.json` are retained in the verification
workspace under `engineering-upgrade/tests/artifacts/performance/20261103-105132-428/`.
Run the commands above to generate local evidence on another machine. Unit checks
also cover histogram/retention bounds and slow-frame classification; synthetic
verifier checks distinguish flat memory, isolated spikes and sustained growth.
Actual missing-shader and invalid-shader game starts both unwound with zero owned
GL objects, including failure after textures/UI had already been initialized.
