# Key, hiding places, and supplied footsteps

The exit begins locked. A seeded brass key appears directly on the carpet along the
connected north route, 90–135 metres north of the entrance. Its gold KEY marker
stays at the minimap edge when outside the local view, and KEY SIGNAL reports
distance and direction. Walk within 1.25 metres to collect it; pickup requires
a clear path and a resident key location. The key disappears, the exit opens,
and the creature becomes enraged. Key state survives chunk eviction but not a
new run. Returning to the entrance with R retains the key, like door progress.

Rage multiplies creature movement speed by 1.35. It also ends the initial spawn
grace period. The creature roars once it is active after pickup, then every
seven seconds while exploring. Pursuit roars use a 1.35-second lower-pitched, low-pass-filtered and throaty
variant of the supplied recording, with soft edges. The sharp jumpscare recording
and impact remain unchanged. Pursuit roars remain positional; they do not take over the mixer like a jumpscare does.
The exit can still be manually closed/reopened after it is unlocked.

H marks a hiding place on the minimap. Wardrobes have been removed. Recesses
are carved into seeded solid wall segments, with a flush entrance, a low
1.35 m roof, and full-height backing integrated as a wall pier. They reuse the
existing yellow wallpaper and floor. Hash-selected wall segments and varied
offsets, widths and depths replace the old periodic corner boxes. Only internal
chunk boundaries qualify, keeping geometry owned by one chunk; doorways, route
centres and the exit vestibule remain clear. Pockets regenerate deterministically.
Both wall orientations are supported, including the camera direction on entry.
Metal ventilation covers now close the entrances. Press E to enter behind a
cover, then look through the gaps between its slats. Ordinary walking cannot
cross the closed cover. The low roof keeps the hidden player crouched; creature
paths reject passages below the height of its lowest pose. See
[ventilation and room details](room-details.md).

Approach and press E to hide; press E again to leave. Entry is refused while an
active creature has an unobstructed view within 24 metres. Hiding places lower
the camera, stop movement and suppress the flashlight while retaining mouse
look. The creature continues searching but cannot sense or catch a hidden
player. Leaving waits if the entrance is obstructed or occupied by the creature.
Pause does not leave cover; a fresh run clears cover and rage.

## Audio

The player uses six isolated contacts extracted from the supplied
`freesound_community-indoor-footsteps-6385.mp3`. The sound is identical across
carpet, damp carpet and tile. Crouch/sprint still affect volume and cadence.

Four contacts from `universfield-heavy-walking-footsteps-352771.mp3` supply the
creature footsteps. Their loudness and stereo position follow listener distance
and facing. Every 0.1 seconds, a bounded ray through world collision counts solid
spans, including live door panels: each wall/door multiplies transmission by
0.38 (up to four barriers). That transmission is applied to active creature
footsteps and roars, in addition to distance attenuation. This is a direct-ray
approximation, not an acoustic propagation or diffraction simulation.

Original MP3s, WAV derivatives, source hashes, and extraction offsets are kept in
`assets/audio/footsteps-player/` and `assets/audio/footsteps-monster/`. The user
supplied both files. Their filenames identify freesound_community and
Universfield; license metadata was not supplied or verified. The older CC0
footstep bank is retained as historical source material and is no longer loaded
by default. Missing banks log a synthesized fallback.

`tools/prepare-footsteps.py` reproduces extraction with a supplied FFmpeg path:
decode mono 22050 Hz PCM16, select separated energy peaks, trim contacts,
normalize to peak 25000, and apply short edge ramps. Python/FFmpeg are only
offline tools; the game still uses WinMM without runtime decoding dependencies.

## Verification

`build.ps1 -Test` includes the new progression suite. It checks five seeds,
locked exit rejection, reachable key placement, one-shot pickup, streaming
persistence, reset, seeded wall pockets, their roofs and solid backing, open/closed door transmission,
increased pursuit speed and hidden-player perception. Audio tests assert equal
player PCM across surfaces and verify supplied monster PCM plus distance and
wall attenuation. Existing world, streaming, movement, threat, model, jumpscare,
and music tests remain enabled.

`tools/verify-progression.ps1` captures the visible key, pickup, locked and opened
exit, wall pockets in multiple seeds, enter/leave cover, and capture/restart after pickup.
Generated evidence lives in `tests/artifacts/progression/`.

The current checks also verify that every key vertex lies below 4 cm and uses
only the brass material. Audio checks distinguish the pursuit growl from the
capture recording. Rendered checks use automated inputs rather than a human
playtest; physical speaker listening is not part of those checks.

Verified on 11 October 2026: all nine native test suites and all ten rendered
progression cases passed, with zero OpenGL errors. The floor key and flush wall
recess captures were visually inspected.
