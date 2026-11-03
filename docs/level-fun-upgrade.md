# Level Fun atmosphere and exploration — 26 October 2026

The second stage keeps its orange walls, green carpet, party furniture and four
gaze-frozen Partygoers. The key search, seeded rolls, 15-box guarantee, mice,
Verity, actor speeds and checkpoint rules are unchanged.

## Abandoned party details

Sparse cell corners contain flattened balloons, cups and napkins, or fallen
garlands and confetti. Some overhead flags are missing or torn. Small winding
music boxes sit on selected existing tables. Every detail is seeded and uses
the existing materials. Near and distant geometry agree on these landmarks.
Floor details stay low and clear of the central cross, gifts and exits; no new
collision, lights, textures or render passes are added.

## Spatial sound

Partygoer footfalls follow actual movement distance, with a soft fabric/carpet
sound every 1.65 metres. Looking at an actor, turning in place or becoming
blocked cannot produce a footstep. Spawn and relocation do not count as motion.
Remaining actor-tagged footstep tails fade when the actor freezes; the existing
waveOut queue can add up to approximately 93 milliseconds of output latency.

Quiet balloon friction and an original three-note toy melody originate at real
seeded balloon stands and music boxes. The first ambient event waits 9–14 active
seconds, and later events are separated by 23–38 seconds. Placement scans at
most 49 nearby cells only when due. Walls attenuate cues. A Partygoer within
eight metres or an active mouse alarm suppresses decoration sounds, with a
five-second recovery period. Pause, death and level changes suspend/reset the
schedule; master volume and mute apply normally.

These sounds are cached procedural samples, synthesized during mixer setup.
They do not change AI hearing, spawn actors or reuse Aranea's calls.

## Finding presents

Press **C** to toggle clues. While searching for the key, the HUD gives distance
and bearing to the nearest unopened present within 32 metres of the player;
the local **Tab** map marks nearby boxes in pink. The bearing is straight-line
direction, not a path through walls. Only loaded, actual presents can be shown.
Searched boxes disappear from the clues immediately. Finding the key retires
gift clues and leaves the exit objective. There is no prediction of a box's roll.

Gift interaction now uses the full camera direction, so looking down at a nearby
floor box still allows **E**. Walls and other props continue to block interaction.

## Verification

`tests/fun_detail_tests.cpp` covers decor bounds, winding, normals, LOD agreement
and streaming. `tests/fun_audio_tests.cpp` covers distance cadence, real prop
anchors, gaze/teleport/pause/reset isolation and mixer tail suppression. The
existing Fun suite covers clue retirement, key guarantees and gift interaction.
`tools/verify-fun-upgrade.ps1` runs real-loop HUD, sound, pause, travel and visual
fixtures. `--fun-ambience-preview` accelerates only the sound schedule when
actors are disabled, for captures; ordinary play uses normal simulation time.

The release build passed the new detail and audio suites, the existing Fun,
Partygoer and complete audio suites, all 12 rendered upgrade cases, and existing
exit, checkpoint and present-mouse scenarios. Rendered checks reported zero
OpenGL errors. HUD/map, toy-box and floor-detail captures were inspected at
960x640. Eight seeds covered 22 decorated corners in the detail suite, with a
maximum of 164,190 full-detail vertices in a sampled chunk.

Compared with the current Desktop baseline, 18 complete Level 0/Parking world
signatures and nine Level Fun collision/light/present/door/objective signatures
matched exactly, with nine chunks per sampled region. These checks establish
regression bounds; no FPS improvement or full manual playthrough is claimed.
