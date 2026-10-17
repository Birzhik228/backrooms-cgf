# Aranea Membri replacement — 12 October 2026

The supplied four-legged Aranea replaces the humanoid Smiler. The original `Scene.glb` and both carpet PNGs are retained unchanged. All 135 mesh parts and 15,220 triangles remain; UV/normal splits produce 11,657 runtime vertices. The source contains no rig or animation. The project's mesh-part rig mapping and analytical leg IK supply the new movement. See [source provenance, cache details and rebuild instructions](../assets/models/aranea/README.md).

## Movement and head animation

Walking staggers the four legs so one swings while three support the body, with brief overlap when all four touch down. Front-left, front-right, rear-left and rear-right use phases `[0, 0.5, 0.75, 0.25]`, 78% stance and a 1.30 m full-cycle stride. Running uses alternating diagonal pairs, phases `[0, 0.5, 0.5, 0]`, 58% stance and a 2.00 m stride. Planted tips move backward relative to the body at the rate of root travel; eased swing arcs and body weight transfer separate the steps visually.

Seven clips contain 220 frames: idle, walk and run; their folded variants; and capture. Actual travel advances a continuous phase, and actual speed controls the walk/run blend. Four baked contact channels replace the two-foot contact handling, triggering the existing heavy recordings as each tip plants. The corrected ordinary locomotion envelope reaches approximately 2.37 m high. Nearby walls and pillars as well as overhead lintels trigger the folded stance. Folding the articulated legs inward gives a baked doorway envelope about 1.15 m wide and 2.15 m high; exact bounds are recorded in the current manifest.

The imported body and skull have different diagonal rest orientations. The baker now aligns the body using its front/rear hip pairs, and separately aligns the skull using the two orbital rims. The eyes become level and the skull faces local −Z; the body, leg strides and face now share that forward direction. This preserves the source GLB while correcting its runtime rest pose. The manifest records both corrections and the reference landmarks.

Navigation turns toward the next route segment before translating, with a 2° alignment tolerance. Reaching a corner ends translation for that frame; the next frame starts its next turn. Rendering uses actual travel direction while moving, avoiding a second visual yaw lag. The moving head remains centered in yaw, with small baked nods and a vertical look adjustment. At rest it can follow a visible player or scan gently. Runtime head yaw remains bounded to ±0.8 rad and pitch to −0.25/+0.20 rad. The same posed geometry feeds normal rendering and the flashlight shadow pass.

The jaw moves subtly during idle and opens for the authored capture; the old Smiler scream clip is not reused. Capture stabilizes the cinematic eye anchor at local height 1.75 m. The camera aims at the current animated head over 0.20 seconds with ease-out; the face then fills the view through a 0.90 m approach and anchored scale/thrust. The front claws reach forward and the head jitters. Random decaying camera shake/roll and the 70°→95° FOV punch complete the default two-second presentation. Change its duration and strengths in `jumpscare.cfg`, or select another file with `--jumpscare-config`. See [jumpscare details](jumpscare.md).

## Materials and integration

The original shared color/normal images and all three glTF material factors are retained through UV-mapped, mipmapped RGB caches. Color is decoded from sRGB; normal data remains linear with source strength 0.635. The new face receives scene lighting and a small capture fill. Permanent Smiler eye/teeth emission is removed.

The `BRARAN3` animation bank is 31,171,529 bytes, approximately 29.73 MiB. Each of the two 512×512 texture caches is 786,448 bytes. Loaders bound the files and reject incompatible or malformed input before use. `aranea-manifest.json` records the current source/bank hashes and baked bounds. Deploy the matching executable, bank and texture caches together. Blender and Python/Pillow are used only for offline regeneration.

The existing enemy perception/navigation, game progression, doors, player movement, music and supplied sound recordings are retained. Animation integrates with their current speed, pause, restart, stoop and capture state. The change does not introduce another enemy, another audio library or a new gameplay mode. Old Smiler source/cache files are kept in the pre-replacement local backup; historical Smiler documentation is not evidence for the Aranea build.

## Validation of the original replacement

The first Aranea replacement passed validation on 12 October 2026, before the subsequent forward-facing correction and configurable capture update described above. For that earlier revision, `build.ps1 -Test -GraphicsTest` passed all nine native suites, the graphics probes (including 28 shadow and 24 slanted-plane AO fixtures), and the lens-noise regression suite. Its executable passed 15 `verify-aranea.ps1` cases with the 15,220-triangle Aranea loaded, music available and zero GL errors. Those model tests measured full/half folded heights of 2.13102/2.19493 m. Evidence is in [aranea-results.json](aranea-results.json); these historical bounds and captures are not measurements of the corrected bank.

The current forward-facing update passed all nine native suites, with final model/encounter retests after capture tuning, all 15 Aranea render cases, all 10 jumpscare cases, and six frame-profile integration scenarios. Actual orbital landmarks confirm neutral head alignment and the capture anchor; live walking/running displacement matches rendered facing (minimum dot product above 0.99998). The old sideways bank fails the new alignment regression. Updated folded heights are 2.14335 m fully folded and 2.21983 m halfway. Current evidence is in [forward-jumpscare-results.json](forward-jumpscare-results.json) and [jumpscare validation](jumpscare.md#validation--12-october-2026).

Rendered frames were inspected for the source shape/materials, leg articulation, head turns, doorway/wall folding and capture framing. A full-height wall fixture exercises the horizontal footprint probes; a live doorway pursuit reaches capture and game-over. The same creature remains driven by the existing navigation body, rather than per-leg collision simulation, so arbitrary corner/turn clearance is not a claim of these checks. No physical listening or manual play session was performed.

- Native suites via `build.ps1 -Test`: existing behavior plus four-leg model/contact handling, stable topology and UVs, normalized normals, supported foot plants, walk/run transitions, independent head motion, door folding, loop/pause/reset behavior and malformed-cache rejection.
- Texture conversion: deterministic outputs, unchanged source file hashes, exact headers/dimensions/payload sizes and normalized tangent vectors.
- Rendering via `tools/verify-aranea.ps1`: walk support/transfer, run support/reach, head left/right, doorway folding, live patrol/pursuit contacts, pause without footfalls, capture, restart and odd-size rendering. All 15 cases loaded 15,220 triangles and reported zero GL errors.
- Visual inspection: assess leg/body articulation, contact timing, texture orientation, head/jaw turns and capture framing from rendered frames. Automated captures alone do not establish a manual keyboard or listening playtest.
- Installation: compare the source baseline before replacing live files, save the previous model in a local backup, verify deployed hashes and smoke-test startup/capture/restart from outside the project directory.

These commands are intended to be run from the project root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -Test
powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\verify-aranea.ps1
```

The existing [Smiler motion results](smiler-motion.md) are explicitly historical. Publication to GitHub is separate from this model replacement.

## Original replacement installation

Installed into the Desktop project after checking all 167 baseline hashes. Thirty files were updated and nine obsolete Smiler asset/tool files were removed after backup. Player settings were preserved. The installed executable was launched from an unrelated working directory and passed a 180-frame capture/restart smoke check: Aranea loaded, music loaded, restart returned to exploration, zero GL errors. Backup: `backups/before-aranea-20261012-213017.zip`. Changes remain local and uncommitted.


17 October tuning: ordinary patrol speed is now 1.2 m/s (was 1.1), and ordinary
chase speed is 3.6 m/s (was 3.3). The existing 1.35 key-rage multiplier remains.
Partygoer speed is unchanged. Door leaves now open away from the interacting
player when fully closed, preserving their signed hinge pose through streaming;
reversing an unfinished animation retains its current direction to avoid a jump.
