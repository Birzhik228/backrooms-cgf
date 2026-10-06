# Requirements and version 0.12 scope

Source: supplied *Computer Graphics Fundamentals* syllabus, PDF pp. 4–5 and 15. This is an educational prototype. Course requirements and the user's preferred gameplay scope are recorded separately below.

| Course requirement | Version 0.12 evidence | Status |
| --- | --- | --- |
| Real-time interactive modern OpenGL | `backrooms.cpp`, C++17, GLFW, GLAD and GLSL 3.30 | Implemented; execution evidence in `verification.md` |
| Scene setup and transformations | World geometry, camera view/perspective matrices, origin rebasing, animated door hinges and articulated creature parts | Implemented |
| Lighting/shading | Blinn-Phong local lights, seeded fluorescent states, attenuation, flashlight cone and fog | Implemented |
| Texture mapping or equivalent effect | Generated environment textures, tangent-space normal maps, UV coordinates and mipmaps | Implemented |
| Buildable repository | Windows build script, Makefile, CMake and dependency instructions | Primary build evidence in `verification.md`; TA-machine build pending |
| 3–6-minute demo | `demo-script.md` | Script drafted; rehearsal and recording pending |
| Report in course template | `report-draft.md` | Draft only; actual template missing |
| Presentation | `presentation.html` | Historical v0.8 deck; update for v0.12 and confirm final course format |
| Attribution and learning | `AI_USE.md`, `learning-guide.md`, `../THIRD_PARTY.md` | Included; student review pending |

The imported creature and its skeletal animation add a supplied mesh and animated transforms to the existing geometry, door, viewing and projection demonstrations. Distinguish the original FBX mesh and scream clip from the new walk/run cycles and integration code. Explain the actual conversion and runtime interpolation; confirm any specific modeling or hierarchy requirement against the final instructor rubric rather than assuming an imported animated figure satisfies it.

| User-requested gameplay | Version 0.12 implementation |
| --- | --- |
| Endless Backrooms exploration | Seeded connected rooms, 25 active chunks and at most 24 prepared neighbours |
| Larger rooms in the yellow Backrooms | 7.5 m cells, 7.5/15/22.5 m footprints and 45 m chunks; classic rooms, corridors, open halls, pillars, dead ends and occasional odd rooms under 3.2 m ceilings |
| One simple roaming threat | One supplied Smiler FBX mesh with its dark body, eyes and teeth; added walk/run cycles, capture animation and doorway clearance handling |
| Closer encounters | The same single threat spawns in reachable unseen space 16–22 m away after the existing 8-second grace period; previously 25–34 m |
| Sight, hearing and pursuit | Wandering, view-cone and line-of-sight detection, movement/door noise, collision-aware pursuit and last-sensed-position search; returns to wandering after about 5 seconds without contact |
| Capture and restart | Roughly 1-second 3D jumpscare, controlled synthesized sting, game-over menu and same-seed Restart Run button; Main Menu prepares a fresh paused run, defeated runs cannot resume; music pauses and master mute applies |
| Smooth first-person movement and stamina | Walk 2.5 m/s, sprint 4.0 m/s, crouch 1.1 m/s; eased mouse look, acceleration/braking, adjustable head bob and stamina |
| Less blur and much darker lighting | Default central blur radius reduced to 0.8 reference pixels; dim ambient/local lighting, adjustable softness/brightness, useful flashlight and sharp HUD |
| Fluorescent lighting | Seeded working, dying and off fixtures; matching fixture flicker and per-light distance attenuation |
| Worn materials and normal maps | Scarred/stained wallpaper, dry/damp carpet, ceiling tiles and hard flooring, with procedural colour and normal textures |
| Actual surface-dependent footsteps | Six bundled recorded variants with surface-dependent filtering and travel-based timing; source/license in `../THIRD_PARTY.md` |
| Spatial ambience without new libraries | Stereo panning and 3D distance attenuation for hum, distant sounds, door effects and creature cues; generated ambience and capture sting through existing WinMM output |
| Animated doors and farther wall-integrated exit | Exit about 270 m north of the entrance; 0.8-second, 90-degree hinge movement; collision follows the panel, obstructed closing reverses, and escape requires walking through the open exit |
| Requested music without new libraries | User-supplied “Numbers” by Temporex, Windows WinMM/MCI playback, separate music toggle/volume and pause/resume without rewinding |
| Menus and settings | Main/pause menus and persistent camera, sound and display settings; music volume defaults to 35% and follows master volume |
| Optimization | Bounded streaming, GPU queue limits, cached sounds and a bounded voice pool; exact measurements in `verification.md` |
| Free exploration with optional clues | Flashlight clues, optional exit, local map and safe changes to unseen rooms retained |

The latest request replaces the v0.11 procedural Watcher with the supplied Smiler model, adds walk/run animation, brings spawn attempts closer and moves the exit farther away. The request for one hostile creature supersedes the earlier no-enemies preference. The earlier room-variety request still supersedes the one-room-type preference. Photo Nextbots, furniture and separate themed districts remain removed. The creature has an initial 8-second grace period; crouching reduces its hearing range while sprinting and nearby door interactions are easier to hear. Walls and closed doors block sight and reduce hearing. Its pursuit is faster than walking but slower than sprinting. Restart resets the seed's run, doors, stamina and enemy state while preserving user settings.

The default camera FOV eases between 78° and 82° during sprint travel; settings can change the base FOV. The exit occupies a wall opening centred at `(6.225, -267)`, approached from `(6.225, -265.8)`. E opens it; walking through the fully open doorway reaches the ending, and Enter resumes exploration. Pausing freezes door and creature motion and pauses the music. Dark lighting and v0.10's reduced blur remain the default.

Before submission: confirm team details, exact deadline, course template, instructor rubric and AI-use interpretation. Test the build on the TA's operating system. Record actual measurements and each student's contributions. The supplied syllabus says to submit via Moodle 2–3 days before the defence, according to the posted schedule. No submission has been made.

Use `verification.md` for checks executed against the current build. Earlier-version passes do not establish this version's behavior. Hidden functional captures differ from a visible FPS benchmark and a live keyboard/mouse playtest. Initial loading and cache misses remain synchronous.
