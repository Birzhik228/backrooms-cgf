# Key, hiding places, and supplied footsteps

The exit begins locked. A seeded brass key rests on the carpet 3-5 connected
rooms before the exit. Follow the gold KEY marker and distance signal; walk
within 1.25 metres with a clear path to collect it. The exit unlocks but remains
closed until the player faces it and presses E. Its pose persists through room
streaming. A new run resets the key; returning with R retains it.

On the first level (Level 0), the guaranteed connected route from the starting
room to the exit approach is now about 123-138 metres, always below 140 metres.
The key lies on that route 3-5 rooms before the exit. HUD distances measure a
straight line and can therefore be shorter than the walk around room corners.
Seeded directions and room variety remain; Fun and Parking keep their existing
exit locations and layouts.

Pickup calls Aranea to that location once, regardless of sound attenuation.
A dormant creature can spawn using the usual hidden, reachable spawn rules.
It runs toward the recorded location at 35% increased pursuit speed and forgets the
call after about five seconds unless ordinary sight or hearing detects the
player. Owning the key keeps the 35% movement speed boost, including after the call
expires. It never refreshes the call or adds periodic rage roars. Normal sighting calls and the jumpscare are unchanged.

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

The current [Aranea bank](aranea-audio.md) supplies separate walk/run contacts
and sighting/pursuit calls. Footfalls use Fantozzi's CC0 sand recordings;
calls combine the selected reference with the supplied ZeroKingFull vocal.
Their loudness and stereo position follow listener distance
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

`build.ps1 -Test` includes the progression suite. The shorter Level 0 route
checks cover 256 seeds for landmark distances, reserved route length, key
placement and seeded variety. Eight routes are walked through streamed collision
geometry to collect the key and open the exit. Fixed regression coordinates
protect the existing Fun and Parking exits. The remaining checks cover five seeds,
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


## 17 October 2026 update

Exit cells now vary in both axes and quadrants with the world seed. The key is
placed 3–5 parent edges toward the entrance, so the reserved route is always
connected and the remaining walk to the door is about 25–40 metres. A normal
launch uses a new seed; explicit `--seed` and hidden verification runs remain
reproducible. Restart/R retain the layout. Native tests traverse the actual
reserved route, collect the key and verify the unlocked exit survives streaming.
The exit interior has near-black wall/floor/ceiling lining and no internal
ceiling lamp. Its external sign and approach light remain visible.

The mouse seeks the player at 1.65 m/s, summons Aranea on its first contact only,
then flees at 2.1 m/s. It uses collision-aware directional steering; a closed
door can still block its approach. Reset clears its one-alarm state. Its tapered
tail now overlaps the body at the root and has a rounded closed tip.
