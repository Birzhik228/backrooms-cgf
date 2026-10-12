# Smiler animation update — 12 October 2026

The supplied mesh and rig now have distinct support, push-off and swing phases. Two-bone ankle IK keeps the stance foot fixed in world travel; pelvis weight transfer and opposing arm motion make individual steps read more clearly. Running uses a longer stride, higher foot lift and a short flight phase. Crouched versions clear low doorways, with maximum full-stoop height 2.198 m.

The head rotates independently using original skin weights and a per-frame pivot. It leads a visual body turn, looks toward a visible player, and scans during idle pauses. It does not track a hidden player's coordinates. Head yaw/pitch and body yaw ease smoothly; pause freezes them and restart resets them. The original scream/capture animation is retained. Flashlight shadow rendering uses the same body transform and posed mesh as the visible actor.

Footstep sound now follows the foot-contact bits in the baked clips rather than a separate distance timer. Exactly one contact per foot occurs in each complete stride. Speed transitions retain a continuous phase; a blocked or paused creature emits no walking steps. Walking and running now use 40 frames per loop, plus 16-frame idle clips and their crouched variants; the complete bank contains 220 frames.

The `BRSMIL2` bank is 11,945,274 bytes, SHA-256 `4941F9B103AB0AE2D8F7E10A80CEA0A4FCE65F90BC8990CA8E9EDA287C6E720D`. Its source FBX hash is unchanged. Original geometry remains 4,466 vertices / 8,728 triangles. Install this bank and the rebuilt executable together. No new runtime dependencies were introduced.

The final `build.ps1 -Test` run passed all nine native suites: progression, world, streaming, movement/settings, threat, model, encounter, audio and music. Model tests measure original skinned foot vertices: world-root movement cancels support-foot movement within 15 mm, support height/lateral drift remain below 10 mm, running has distinct flight, each foot contacts once per cycle, and turning the head leaves leg geometry unchanged. Loop continuity, pause determinism, normal normalization, shape/door bounds and invalid-file rejection also pass.

Thirteen final game checks passed for walk support/transfer, run support/flight, both head directions, stooping, live patrol/pursuit, pause, capture, restart and odd-size 1919x1027 rendering. Every case loaded the imported mesh and reported zero GL errors. Live pursuit produced four contact cues in 100 fixed simulation frames; the longer live fixture produced fifteen cues. The paused case produced zero cues. [Commands and logs](smiler-motion-results.json). Captures and offline pose previews were inspected in `tests/artifacts/motion`.

These are automated callback and framebuffer checks, not a physical headset or manual keyboard playtest. The recent lighting, AO and lens fixes were copied from the active Desktop source and their shader hashes remain unchanged. Publication to GitHub was not part of this request.

Installation verified all 140 live baseline hashes and updated 15 files. Backup: `backups/before-smiler-motion-20261012-205315.zip`. A capture/restart smoke test launched from outside the project folder loaded the new bank and music, reset the run and reported zero GL errors.
