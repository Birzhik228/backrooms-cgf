# BACKROOMS / Threshold v0.10

**Educational prototype.** Team members, course group, instructor and date: to be supplied. Transfer this draft into the official course template when available. Review descriptions against the delivered source and record student contributions. See `AI_USE.md`.

## Introduction

The project combines procedural generation and a modern OpenGL renderer to create a connected interior that grows around the player. Worn yellow wallpaper, carpet and unreliable fluorescent fixtures establish the Backrooms setting. Smooth first-person movement, lightly softened imagery and spatial sound accompany solitary exploration, with interactive doors, optional flashlight clues and an exit. Environmental tension replaces enemies.

## Background

The supplied lectures cover triangle meshes and GPU buffers [1], camera frames and perspective projection [2], local lighting and the halfway-vector model [3], texture objects and mipmaps [4], and procedural methods [5]. The project combines these concepts in an interactive application. Its maze connectivity rule is a project design rather than a copied lecture algorithm.

## Procedural world and streaming

Each 7.5-metre cell belongs to a 6 × 6-cell chunk spanning 45 metres. Every non-origin cell reserves an opening toward a neighbour closer to the origin. Following these links repeatedly reaches the origin, which establishes global connectivity. Seeded optional openings add variation. Adjacent cells evaluate their shared edge consistently, including across chunk boundaries.

The yellow architectural style spans classic rooms, repeating corridors, open halls, pillar halls, dead ends and occasional odd rooms with hard floors. Procedural partitions vary across 7.5, 15 and 22.5 metre footprints beneath uniform 3.2 metre ceilings, making rooms 25% wider and deeper than in v0.9. There are no separate themed districts or furniture. A single floor quad and ceiling quad span each whole room, reducing repeated geometry. Solid walls, pillars and moving door panels have collision checks. Reserved connecting routes remain traversable. Temporary changes to distant unseen rooms preserve connectivity and reset when their chunk unloads.

A radius of two chunks yields 25 active chunks. A bounded outer ring holds at most 24 prepared neighbours. Each frame prepares one missing nearby chunk and can upload its mesh before activation. Crossing a boundary promotes prepared chunks, and unchanged geometry can be reused when the player turns back. Distant entries are evicted. Initial loads, teleports and cache misses use synchronous generation so collision does not depend on missing floors.

World positions use double precision. Rendering subtracts a nearby origin before conversion to GPU floats, limiting distant-coordinate precision loss. GPU frame fences limit queued rendering work. Entity simulation, photo textures and billboard rendering remain removed. Measurements belong in the verification record, with their hardware and method.

## Camera, movement and collision

Yaw and pitch define the first-person camera; mouse deltas feed targets that are exponentially smoothed using elapsed time. View and perspective matrices transform local geometry into clip coordinates. The default vertical field of view eases from 78° to 82° while actually sprinting. Crouching eases eye height from 1.65 to 1.08 metres. Adjustable head bob follows travelled distance rather than idle input.

Walking reaches 2.5 m/s, Shift movement 4.0 m/s and crouching 1.1 m/s. Velocity approaches the requested movement gradually, and braking slows the player after input stops. Crouching takes priority over sprinting. Collision checks the player's footprint against walls and subdivides travel to prevent tunnelling. Sliding along obstacles preserves movement parallel to a wall. Pausing or resetting clears movement velocity.

Stamina starts at 100 and drains at 18 points per second during actual sprint travel. It recovers at 14 points per second after a 1.5-second delay; exhaustion blocks sprinting until 22 points have recovered. Pausing freezes this state. Returning to the entrance or starting a new seed restores stamina.

## Rendering and sound

Procedurally generated wallpaper, carpet, acoustic-ceiling and hard-floor textures use repeating UVs and mipmaps. Scars, scuffs and peeling patches are baked into a wallpaper sheet covering 12 metres horizontally. The wall shader adds the rendering origin modulo 180 metres to local coordinates: 180 is a common multiple of the 12-metre sheet and 45-metre chunk. This bounded phase keeps marks fixed when the origin changes without passing huge coordinates to the GPU. The shader also adds damp staining near the floor. Four corresponding normal maps are generated from physical-height fields using finite differences. A tangent basis derived from position and UV derivatives transforms the sampled normals into the lighting coordinate system, including reversed floor/ceiling UV directions. Damp carpet uses a darker colour, flatter normal detail and stronger highlights. These effects change shading rather than mesh geometry or collision.

The fragment shader combines textured colour with local diffuse and specular lighting, per-light attenuation, a flashlight cone and fog. Seeded fluorescent fixtures have working, dying or off states and stable phases. The same time envelope drives a fixture's visible glow and its selected point-light contribution, so changing the nearest-light list does not alter its flicker phase. The default ambient fill remains dark. Local lighting does not test whether a wall blocks a light.

The scene renders to an offscreen framebuffer. A 13-sample soft-focus postprocess, analog grain and vignette provide a restrained camera treatment within one fullscreen pass. The default central blur radius is now 0.8 reference pixels, down from v0.9's 1.7; the edge contribution changes from 0.7 to 0.3. Camera softness and brightness remain adjustable, and existing preferences are preserved. The UI draws afterward, keeping text sharp. V toggles the minimal HUD. This display effect does not record video.

The existing Windows WinMM output API plays a stereo mix without a new audio library. Six recorded shoe-footfall variants come from GboxMikeFozzy's CC0 collection on OpenGameArt, converted and filtered for indoor playback; source details and changes appear in `../THIRD_PARTY.md`. Cached variants use different filtering for dry carpet, damp carpet and hard floors. Steps follow actual distance rather than held movement keys, so pushing into a wall does not keep creating walking sounds. Crouch steps are quieter and sprint steps have heavier impacts.

World-space sources use listener-relative stereo panning and three-dimensional distance attenuation. The nearest active fluorescent fixture hums, while seeded distant creaks and drips, door effects and stance rustle are synthesized placeholders. A subdued drone and occasional electrical stinger respond to darkness, unstable lamps and odd rooms. The mixer uses cached clips and at most 12 overlapping one-shot voices. M mutes the mix; master volume is adjustable. The audio-preview command exports a repeatable 12-second stereo WAV without a graphics window. It is a generated demonstration, not a recorded playthrough.

The user supplied a music file for “Numbers” by Temporex. The original is preserved as `assets/audio/music/numbers-temporex.m4a`; a locally converted `numbers-temporex.mp3` in the same directory is used for playback because this computer's Windows MCI decoder does not accept the original file. Windows WinMM/MCI plays the track separately from the spatial sound mixer, without a new runtime audio library. It is background music, not a source positioned inside a room. A separate music toggle and volume setting control it; the default music level is 35%, multiplied by master volume. Pausing or muting pauses playback without rewinding. The environmental audio-preview WAV excludes the song. Track attribution and provenance are recorded in `../THIRD_PARTY.md`.

## Interaction and current scope

The exit is built into a real wall aperture centred at `(6.225, -87)`, about 90 metres north of the entrance. Its approach point is `(6.225, -85.8)`. The HUD reports distance and a bearing relative to the camera. Pressing E while facing the nearby door starts its opening animation; the player must then walk through the fully open doorway to reach the ending. Interaction alone no longer activates escape. The ending pauses play. Enter resumes exploration without truncating the generated world.

Ordinary doors and the exit use a hinged panel that eases through 90 degrees over approximately 0.8 seconds. Visual geometry and collision share the same hinge pose, and a closing panel reverses if the player obstructs it. Pausing freezes animation. The moving leaf is drawn separately from the static room mesh, so its animation does not require rebuilding the whole room. At most 128 opened-door overrides survive unloading; older overrides return to their seeded state when regenerated. Door changes do not remove the reserved connected routes.

Clickable main and pause menus expose play/resume, settings, a new seed, return to the entrance and quit. Validated settings persist in `settings.cfg`: mouse sensitivity, master volume, music on/off, music volume, brightness, field of view, head bob, camera softness and VSync. The map, flashlight clues, wireframe view and performance diagnostics remain available. There are no Nextbots or other entities, furniture or separate themed districts in version 0.10.

## Testing and results

The verification plan covers deterministic room categories, larger-room connectivity, animated door/collision safety, bounded streaming, camera and stamina transitions, settings validation and the stereo audio mix. Graphics checks target normal-mapped materials, matching fluorescent states, door opening/closing, physical exit crossing, reduced blur, menus, distant coordinates and streaming. Music checks should cover decoding, volume and pause/resume. This paragraph describes intended coverage, not a claim that every case passed. Exact executed cases and results are maintained in [verification.md](verification.md); v0.9 results alone do not establish v0.10 behavior.

Hidden functional graphics captures do not establish visible gameplay FPS. Frame-time comparisons must state the machine, resolution, route and collection method. A live keyboard/mouse playtest and a build on the defence machine remain necessary.

## Limitations

Architectural motifs repeat. Lighting has no shadow maps or global illumination, and normal maps do not change silhouettes. Spatial sound uses stereo panning and distance attenuation without HRTF filtering or wall occlusion. The exit shows an ending screen rather than an exterior scene. Settings persist, but gameplay progress does not. Active geometry and change records are bounded, and changes eventually reset after eviction. Initial loads and cache misses can still cause synchronous work. Numerical ranges remain finite. The prototype needs student review and an instructor-approved submission scope.

## References

References are supplied lecture PDFs; page numbers mean PDF page positions.

1. *L-4-2_Models*, pp. 4–20.
2. *L-5-1_Viewing*, pp. 3–22, 36–47.
3. *L-5-2_Shading*, pp. 14–32, 51–70.
4. *L-8_TextureMapping*, pp. 25–47.
5. *L-10_ProcMeths*, pp. 5–28, 62–103, for procedural concepts rather than the specific maze algorithm.
6. *Computer Graphics Fundamentals syllabus*, pp. 4–5, 15.

