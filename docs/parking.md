# Stage 3: Underground Parking

The sequence is **Level 0 -> Level Fun -> Parking**. Walk through Level Fun's
unlocked exit to reach Parking. The new stage has its own session checkpoint:
death restarts Parking with fresh circuits and a short grace period. R returns
to its entrance, retaining restored circuits. Neither checkpoint nor power is
saved across application launches, consistent with the existing levels.

## Darkness and flashlight

The monster has no visible body: pairs of pale eyes float through the dark
garage and approach the player. Contact in darkness triggers a two-second
close-up jumpscare with an electrical shriek, a lunging face, camera shake and a blackout,
then the restart screen. Disabling jumpscares in the existing config retains
contact defeat without the presentation. Aim an
enabled flashlight directly at the eyes (within 18 metres) to disperse them.
Looking without the light does not help. Concrete columns and cabinet walls
block the beam. Dispersed eyes cannot immediately respawn; at most three pairs
exist at once. Pause freezes their movement, spawning and contact.

The first eyes appear after two seconds, preferentially at visible edges of
the view outside the flashlight beam. They remain still for 0.65 seconds on
arrival, then chase at **4.8 m/s**, faster than the player's **4.0 m/s** sprint.
Dispersing a pair gives a 4-6 second respite before it can spawn again. Running
straight away cannot outrun them; use the flashlight or a lit refuge.

The illuminated entrance and cabinet approaches are small safe refuges.
Restoring a circuit makes its entire sector safe and brings its fittings back
on. The HUD distinguishes safe light from darkness. There is no battery drain.

## Objective and environment

- Find cabinets A, B and C using the circuit signal and Tab map.
  Their pairwise separation is roughly 80-130 metres, depending on seed.
- Face a cabinet from its front, approach within 1.8 m and press E.
- Each cabinet permanently restores its sector for the current run. Progress
  survives chunk eviction and prefetching. Any activation order works.
- After all three circuits are on, follow EXIT SIGNAL, press E at the service
  door and walk through it. The completion screen allows continued exploration.

The garage contains concrete columns, caution stripes, parking bays, wheel
stops, low-poly sedans, ceiling beams, service pipes and specular damp patches.
Most ceiling lights are initially off, with a few failing emergency fittings.
Unpowered emergency lights and ambient fill have been dimmed further. Cars
occupy about one in eleven bays/cells, down from one in four.
Powered fittings become steady and brighter. Continuous drive and cross aisles
connect the landmarks. Parking uses the existing bounded chunk streamer but
does not shuffle its layout behind the player. No Aranea, Partygoers, mice,
presents, brass key or infinite-stamina reward carry over from earlier stages.

All new meshes, materials and eye visuals are authored procedurally in code;
no models, images, audio or map layouts were downloaded from another game.

## Reference research, 18 October 2026

- [Level 1 / Habitable Zone](https://backrooms-wiki.wikidot.com/level-1): visual
  reference for a concrete parking environment, repeating supports and dampness.
- [Escape the Backrooms: Smiler](https://escapethebackrooms.fandom.com/wiki/Smiler):
  describes darkness encounters in the parking area and light-repelling behavior
  in other encounters. Implementations differ between games/levels. This stage
  follows the user's requested combination: parking, floating eyes, flashlight
  dispersal. It is an original variation, not an exact recreation of a map.

## Development and checks

Start directly: `Backrooms.exe --demo --parking`.

`common/parking_geometry.h` generates meshes; `world.h/.cpp` own circuit state
and exit gating; `common/darkness.h` owns eyes, beam occlusion, safety, spawning
and movement. `backrooms.cpp` integrates transitions, interaction, rendering,
audio cues, HUD and restart. Material IDs 70..91 are reserved for this stage.

- `tests/parking_tests.cpp`: seeded geometry, winding, clear aisles, reachable
  cabinet approaches, interaction rejection, streaming/prefetch persistence,
  lamp restoration, exit gates, flashlight cone/occlusion, darkness contact,
  pause, refuges, powered sectors and isolation from previous levels.
- `tools/verify-parking.ps1`: rendered arrival, eyes with light on/off/aimed
  away, contact/restart, three circuits, completion, locked exit, pause, guide
  and streaming scenarios. Every case checks OpenGL errors.
- `tools/verify-fun.ps1`: includes both Level 0 -> Fun and Fun -> Parking.

Fixtures: `--darkness-preview` places one pair ahead of the camera;
`--auto-breakers` visits all cabinets through the normal E interaction;
`--auto-breakers --auto-exit --auto-walk` exercises completion. These are
development-only CLI options and are not used in normal play.

Verified locally on 18 October 2026: the game builds successfully; eleven
native gameplay/world/audio suites pass, including Parking/Darkness; both
graphics suites pass on the NVIDIA RTX 3050 OpenGL 3.3 renderer. All twelve
Parking scenarios and sixteen Fun/regression scenarios pass with zero OpenGL
errors. Arrival, visible/dispersed eyes, powered cabinets, the guide and the
completion screen were inspected in rendered captures.

The pre-existing Windows music lifecycle test still fails to initialize MCI
(error 277); `build.ps1 -Test` consequently returns failure for that suite.
It is not counted as a pass. Environmental/gameplay audio tests pass. Automated
fixtures and screenshots do not establish a physical listening test or a
complete manual playthrough.

Follow-up verification on 18 October: all fourteen updated Parking scenarios
pass, including natural spawning without a forced actor, jumpscare presentation,
delayed game over and checkpoint restart. The Parking native suite now checks
that a pursuing pair overtakes a 4 m/s sprint along a clear aisle, as well as
cabinet separation. Encounter timing tests and the three exit/transition
regression scenarios pass. Rendered captures confirm the dimmer garage and
the close-up face.

The Parking sting now has its own synthesized sound: a bass impact, falling
dissonant tones and a distorted hiss fading over 1.7 seconds. Aranea retains
its existing scream. The new sound uses the same exclusive centered capture
mix, master volume, mute and pause behavior. No external audio asset is needed.
Audio tests export the actual stereo mix to
`tests/artifacts/darkness-jumpscare.wav` for listening.
