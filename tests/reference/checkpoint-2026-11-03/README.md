# Stable checkpoint — 3 November 2026

This reference set preserves the current visuals and movement before further
code simplification. The containing checkpoint commit includes all pending game,
architecture, resource, tuning, diagnostics and build changes. This checkpoint
adds references and documentation; it does not modify game behavior.

## Saved references

| Fixture | What it records |
| --- | --- |
| [Level 0 sprint](level0-sprint.png) | Movement, camera, stamina and first-level materials |
| [Level 0 crouch](level0-crouch.png) | Crouched movement and camera height |
| [Pause](paused.png) | Paused world and menu |
| [Aranea](aranea-facing.png) | Creature pose and facing |
| [Level Fun](level-fun.png) | Furniture, lighting and materials |
| [Parking](parking.png) | Third-level lighting and geometry |
| [City ending](city-ending.png) | The city cinematic at 37 seconds |

Every PNG is a lossless conversion of the in-engine BMP capture; pixel data was
checked against its source. All seven runs exited successfully with zero OpenGL
errors and zero application-owned GL resources after Game destruction. All images
were inspected. Four cases also exactly match the earlier tested build's pixels
and non-timing CSV data.

[manifest.json](manifest.json) records exact arguments, seed, resolution, renderer,
executable hash and reference-file hashes. CSVs retain non-timing per-frame data
for movement/camera/creature comparisons; the cinematic case has no gameplay CSV
rows. [validation.json](validation.json) separates these fresh checks from the
previous 28 native, four OpenGL and 48 runtime cases, 27 transitions and 600-second
Parking soak on the identical source/executable. [source-hashes.json](source-hashes.json)
covers the 286 pre-checkpoint files, excluding this new reference directory.
Raw working-copy hashes can differ after Git converts LF/CRLF line endings.

## Comparing a later refactor

Build the later version, then use each manifest entry's `args` with additional
`--capture tests/artifacts/checkpoint/<name>.bmp` and
`--profile tests/artifacts/checkpoint/<name>.csv` options. Run sequentially: game
instances share `backrooms.log`. Hidden runs ignore personal settings and use
shipped balance/jumpscare settings. The fixtures use deterministic simulation
steps. Compare pixel content, not PNG versus BMP file bytes, and omit `*_ms`
columns from the new CSV before comparing it to the reference CSV.

Use the same GPU/driver/settings for strict image comparison. A different GPU can
produce rendering differences; review them instead of blindly overwriting these
references. This checkpoint does not establish operation on another physical PC.

The local annotated tag is `checkpoint-2026-11-03`. Inspect its history with
`git show --stat checkpoint-2026-11-03`. For a separate reference checkout use
`git worktree add --detach ../backrooms-checkpoint checkpoint-2026-11-03`; this
keeps ongoing work in the main directory intact. Build that checkout normally.
Executables, personal settings and raw logs remain outside Git. A copy of the
verified executable is retained in the local task workspace's
`checkpoint-20261103/`, alongside the source archive made after the commit.
