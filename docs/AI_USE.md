# AI assistance and authorship disclosure

On 26 October 2026, Codex assisted with the requested level selector, native
menu layout, fresh-level transitions and validation. At the user's follow-up
request, the selector was moved into the Escape menu and Ctrl retained only
its crouch action. Level 0's seeded key/exit route was
shortened to under 140 metres while preserving later levels' layouts.
See [level selection](level-selection.md) for controls and verification.

On 19 October 2026, the user requested more atmosphere on Level 0 and farther
Aranea spawning. Codex assisted with sparse world-aware ambient timing, soft
cached ventilation/rustle synthesis, muffled sequences from the existing
credited soil recordings, actual-creature presence hints, danger suppression,
24–32 metre spawn selection with initial-position clearance, and automated
audio/world/gameplay checks. No new third-party recording was downloaded.
See [Level 0 atmosphere](level0-atmosphere.md).

OpenAI Codex assisted with this version 0.12 educational prototype, including substantial C++/GLSL code, procedural asset code, build support and documentation. Codex also assisted with testing and debugging and generated the historical v0.8 presentation material. Consult `verification.md` for the checks actually performed. The user selected the Backrooms theme and requested effectively endless generation and free exploration with optional clues, subsequently adding one hostile creature.

Earlier requests added an exit, lighting, room variety, furniture, non-hostile figures, separate furnished areas, varied room proportions and movement sounds. In version 0.4 the user supplied five photos and requested pursuing Nextbot-style entities. Codex assisted with image-format conversion, upright billboards, collision-aware pursuit and caught/retry behavior. Version 0.5 increased movement and chase speed. Version 0.6 added special districts and water footsteps, preparation of nearby chunks, bounded mesh reuse and GPU frame queue limits.

For version 0.7, the user requested removal of all Nextbots and entities, one room type, slight camera blur, slower/heavier walking and optimization. Codex assisted with removing pursuit and photo rendering, reducing the world to the classic yellow style, acceleration/braking, revised camera and sound treatment, validation and these documentation changes. The previous photo assets and special districts are absent from this version's runtime. The earlier photographs were user-supplied; Codex does not claim authorship of them.

For version 0.8, the user requested stronger blur, actual footsteps and scars on the walls. Codex assisted with stronger scene postprocessing, procedural wallpaper wear, recorded footstep integration and updated documentation. The user subsequently requested less blur and much darker lighting. Footstep recordings come from GboxMikeFozzy's CC0 collection on OpenGameArt, credited in `../THIRD_PARTY.md`. Codex adapted and integrated those recordings but did not record the original sounds.

For version 0.9, the user requested smooth first-person controls, stamina, normal-mapped worn surfaces, working/dying/off fluorescent lights, doors, spatial and surface-dependent sound, menus/settings and varied seeded room layouts. The user explicitly confirmed that enemies should remain absent and tension should be environmental. This superseded the earlier one-room-type preference. Codex assisted with these implementations, generated new placeholder ambient/door sounds, retained the existing WinMM output API without adding an audio library, and revised the documentation. Furniture and photo entities remained removed, with slower movement and darker default lighting retained.

For version 0.10, the user requested an exit generated inside a wall, opening and closing animations, larger rooms and “Numbers” by Temporex. The user supplied the music file. Codex assisted with 7.5-metre cells, 45-metre chunks, a wall aperture and hinged exit, shared door animation/collision, physical doorway crossing to trigger escape, Windows WinMM/MCI music playback, local format conversion, separate music settings, reduced camera softness and texture mapping that stays fixed across origin shifts. Codex did not compose, perform or record the supplied song. Its provenance and attribution are recorded in `../THIRD_PARTY.md`. No new audio library or runtime download is required. The existing `presentation.html` still describes v0.8 and is historical rather than an updated v0.10 deck.

For version 0.11, the user requested one wandering creature that chases on sight or sound, gives up after losing the player, and causes a jumpscare and restartable game-over screen on capture. This superseded the earlier no-enemies preference. The user supplied a screenshot as a visual reference for a creature they identified with MONOCHROME/Room. Codex assisted with a custom volumetric model, elongated articulated limbs, hollow white eyes, procedural animation and an editable OBJ/MTL export. No original Roblox model or texture was extracted. Codex also assisted with bounded local pathfinding, perception, last-position search, capture/restart integration, tests and documentation. The new recognition, creature-step and capture sounds are generated in project code and cached before gameplay; no new sound library or external recording is used for these cues. Player movement speed, darker lighting, reduced blur, recorded footsteps and the supplied soundtrack remain part of the project. The historical v0.8 presentation still needs updating before a v0.11 demonstration.

For version 0.12, the user supplied `smiler-backrooms.zip`, containing `source/Zombie Scream.fbx`, and requested that model with walking and running animations, closer enemy spawns and a farther exit. Codex assisted with offline FBX conversion, added skeletal locomotion poses and a runtime animation cache, integration of the supplied scream animation, spawn-range adjustment from 25–34 m to 16–22 m, relocation of the exit to about 270 m north of the entrance, tests and documentation. The supplied mesh, skeleton and original scream clip are not claimed as Codex-authored or student-authored assets. Blender is used for offline conversion; building and playing do not require it. The archive establishes the supplied file, not its original creator or license. See `../THIRD_PARTY.md` and the model notes for provenance. The historical v0.8 presentation needs updating before a v0.12 demonstration.

This is not a claim that the student independently authored the implementation. The student must inspect, adapt and understand the work, document their own contributions and disclose assistance in any permitted submission.

The supplied syllabus, p. 5, permits disclosed, scoped assistance including brainstorming, explanation, boilerplate, refactoring and debugging. It prohibits presenting AI-generated solutions as one's own or bypassing required learning. A generated prototype of this scope needs the instructor's interpretation or approval before inclusion in an assessed submission. Disclosure alone does not establish that the full prototype is permitted.

Student contribution log: add dated entries naming code reviewed, experiments performed, changes authored and tests personally repeated. Leave claims of independent work out until they are accurate.

## Aranea replacement — 12 October 2026

The user supplied `rec-room-backrooms-entity-62-aranea-membri.zip` and requested replacing the current creature with four-legged spider-like movement and head animation. Codex assisted with inspecting the unrigged GLB, authoring four inverse-kinematic leg chains and a separate head/jaw articulation, generating grounded walk and diagonal chase cycles, retaining the supplied mesh topology/UVs/textures, runtime animation/contact integration, and validation. The supplied source geometry and textures are not claimed as project-created assets. Runtime dependencies remain unchanged. See [model notes](../assets/models/aranea/README.md) and [implementation](aranea-motion.md).

For the follow-up forward-facing and jumpscare update, Codex helped correct embedded body/skull rest axes, make navigation turn before moving, add configurable camera/lunge timing and decaying random shake, and author capture head/claw motion. Tests check actual eye landmarks, forward displacement, capture timing and restart. The supplied source GLB remains unchanged; authored animation and presentation code are project additions.

On 13 October 2026 the user requested Aranea walking, running and sighting
sounds, supplied a YouTube reference and specified that the creature runs on
the ground. Codex assisted with excerpt selection, transient isolation, ground
foley layering, pitch/EQ/envelope processing, separate cached sound banks,
visibility-triggered vocal timing and automated audio/gameplay validation.
Original source recordings are credited in THIRD_PARTY.md and the Aranea asset
manifest. Edited reference audio is not claimed as an original performance.

In the subsequent soil/call correction, Codex helped replace the hard contacts
with Fantozzi's CC0 sand recordings, soften their attack and add low ground
pressure and rustle. Detection/pursuit vocals blend the selected reference with
the supplied ZeroKingFull recording. Revised vocal distance, voice priority,
queued reacquisition and sustained-pursuit timing address quiet/missed calls.
The audio suite, actual mixer exports and five automated game cases verify
behavior; no physical listening test or exact perceptual match is asserted.

## Parking and darkness - 18 October 2026

At the user's request, Codex researched Backrooms parking references and
implemented an original third stage after Level Fun. Assistance includes
procedural garage geometry/materials, electrical circuit progression, the
user-specified floating-eye darkness and flashlight defense, level transitions,
HUD/guide/restart integration and native/rendered verification. New geometry
and eye visuals are authored in code; no third-party game assets were copied.
Research sources and implementation boundaries are listed in `parking.md`.

## Performance and navigation - 20 October 2026

Codex investigated reported third-stage heating, measured local frame/process
costs, implemented configurable frame pacing and stable lamp shadow-cache slots,
and corrected wall-adjacent/corner navigation. Assistance included regression
and rendered checks and the measurements in `performance-2026-10.md`. No friend's
hardware or temperature measurements were available; no thermal guarantee is made.

## Elevator test ending - 26 October 2026

At the user's request, Codex authored an original in-engine animated ending:
procedural elevator/lobby/Backrooms geometry, scripted camera and door motion,
procedural materials and lighting, synthesized placeholder cinematic sounds,
playback controls, Parking-exit integration and regression checks. Existing
project gameplay and preferences were preserved. No third-party scene or new
library was used. The scene is a prototype, not a prerendered video.


## City ending revision - 26 October 2026

At the user's request, the test ending now exits into a daylight city instead of
returning to the yellow rooms. Codex authored bounded procedural street/building
geometry, pedestrian walk cycles, moving vehicles, a longer camera path, city
sound synthesis and regression checks. All city assets are code-generated and
reuse the existing renderer/audio dependencies. The scene remains a scripted
in-engine prototype; the ordinary streamed levels are not replaced by a city map.

### City sunlight - 26 October 2026

Added an original analytic sky sun and directional shadow rendering to the
city ending. Cached static and animated depth maps use two fixed projections;
city geometry and pedestrian/car transforms supply the shadow silhouettes.
No external shaders, assets or libraries were introduced. Rendered occlusion,
pause/cache lifetime and replay tests accompany the change.
