# Parking atmosphere and wayfinding — 26 October 2026

The third stage keeps its dark garage, three electrical circuits and floating
eyes repelled by the flashlight. Car placement, drive/cross aisles, collisions,
lights, safe refuges, eye behavior and exit requirements remain unchanged.

## Landmarks and circuit feedback

Sector letters A/B/C appear on all four column faces. A painted letter and arrow
mark the approach to each actual electrical cabinet. They identify the local
approach, not a globally calculated route. Sparse ceiling grilles, low cable
coils and wet patches beneath pipes add maintenance traces using existing
materials. Near and distant meshes agree on their placement. Details introduce
no extra obstacles, lamps, shaders or textures.

Three HUD badges show the live ON/OFF state of circuits A/B/C. The nearest
unfinished circuit is highlighted and framed on the map. The distance/bearing
signal points to the same target; looking up or down no longer changes its
horizontal bearing. Restoring a circuit updates the checklist immediately and
announces the safe sector. Once all three are on, the exit remains the objective.

## Industrial sound

Pipe ticks originate at existing overhead pipes. Drips come from the pipes over
seeded wet patches, and ventilation comes from actual ceiling grilles in powered
sectors. These original procedural sounds are cached at audio initialization.
The first event waits 8–13 active seconds; later events are spaced 17–29 seconds
apart. A missing suitable prop delays the attempt instead of inventing a source.
Placement checks at most 49 nearby cells per attempt, with wall attenuation.

A circuit activation plays one spatial relay/startup cue from its cabinet.
Returning to the entrance with R retains power and does not replay the cues.
Checkpoint restart resets circuit progress; entering another level resets the
director and mixer. Pause freezes scheduling, and death suppresses ambience.

Eyes within eight metres quiet decorative sound, followed by five seconds of
recovery. Actual eye arrival, dispersal and capture cues remain audible, as does
the power confirmation. No footsteps or extra body are invented for the eyes.
All sounds obey the normal master volume and mute controls.

## Verification

`tests/parking_detail_tests.cpp` covers signs, local arrows, detail bounds,
winding, LOD agreement and streaming. `tests/parking_audio_tests.cpp` covers
actual source anchors, powered-only ventilation, one-shot circuit edges,
pause/reset/isolation and mixer behavior. The existing Parking suite includes
circuit targeting, connected aisles, power persistence and Darkness rules.

`tools/verify-parking-upgrade.ps1` exercises the HUD, partial/all power, sound,
pause, travel, isolation and visual fixtures through the real game loop.
`--parking-ambience-preview` advances only the sound schedule, with enemies
disabled, for repeatable captures. Normal play uses ordinary simulation time.

The release build passed the new detail/audio suites, the existing
Parking/Darkness suite, the complete audio suite and Fun audio regression tests.
All 13 upgrade scenarios and all 14 existing Parking scenarios passed with zero
OpenGL errors, including flashlight dispersal, capture/restart, circuit power,
completion, pause and streaming. Signs, cabinet marking, grille, pipe patch,
cable and circuit HUD captures were inspected at 960x640.

The geometry sweep covered eight seeds, 38 grilles, 12 wet patches, 26 cables
and 24 cabinet arrows, with at most 53,328 vertices per sampled chunk. Against
the current Desktop baseline, all 18 complete Level 0/Fun signatures and 54
Parking collision/light/door/circuit signatures matched exactly. Parking checks
covered unpowered, A-only and fully powered states at six regions per seed.
These are regression checks, not an FPS benchmark or a full manual playthrough.
