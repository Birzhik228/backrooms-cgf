# Requirements and version 0.10 scope

Source: supplied *Computer Graphics Fundamentals* syllabus, PDF pp. 4–5 and 15. This is an educational prototype. Course requirements and the user's preferred gameplay scope are recorded separately below.

| Course requirement | Version 0.10 evidence | Status |
| --- | --- | --- |
| Real-time interactive modern OpenGL | `backrooms.cpp`, C++17, GLFW, GLAD and GLSL 3.30 | Implemented; execution evidence in `verification.md` |
| Scene setup and transformations | World geometry, camera view/perspective matrices, origin rebasing and animated door hinges | Implemented |
| Lighting/shading | Blinn-Phong local lights, seeded fluorescent states, attenuation, flashlight cone and fog | Implemented |
| Texture mapping or equivalent effect | Generated environment textures, tangent-space normal maps, UV coordinates and mipmaps | Implemented |
| Buildable repository | Windows build script, Makefile, CMake and dependency instructions | Primary build evidence in `verification.md`; TA-machine build pending |
| 3–6-minute demo | `demo-script.md` | Script drafted; rehearsal and recording pending |
| Report in course template | `report-draft.md` | Draft only; actual template missing |
| Presentation | `presentation.html` | Historical v0.8 deck; update for v0.10 and confirm final course format |
| Attribution and learning | `AI_USE.md`, `learning-guide.md`, `../THIRD_PARTY.md` | Included; student review pending |

The prototype does not currently claim an articulated character hierarchy. Geometry placement, viewing and projection demonstrate transformations. Confirm any additional modeling requirements against the final instructor rubric.

| User-requested gameplay | Version 0.10 implementation |
| --- | --- |
| Endless Backrooms exploration | Seeded connected rooms, 25 active chunks and at most 24 prepared neighbours |
| Larger rooms in the yellow Backrooms | 7.5 m cells, 7.5/15/22.5 m footprints and 45 m chunks; classic rooms, corridors, open halls, pillars, dead ends and occasional odd rooms under 3.2 m ceilings |
| No entities; environmental tension | No photo Nextbots, figures or pursuit; darkness and unstable lighting drive a drone and occasional stinger |
| Smooth first-person movement and stamina | Walk 2.5 m/s, sprint 4.0 m/s, crouch 1.1 m/s; eased mouse look, acceleration/braking, adjustable head bob and stamina |
| Less blur and much darker lighting | Default central blur radius reduced to 0.8 reference pixels; dim ambient/local lighting, adjustable softness/brightness, useful flashlight and sharp HUD |
| Fluorescent lighting | Seeded working, dying and off fixtures; matching fixture flicker and per-light distance attenuation |
| Worn materials and normal maps | Scarred/stained wallpaper, dry/damp carpet, ceiling tiles and hard flooring, with procedural colour and normal textures |
| Actual surface-dependent footsteps | Six bundled recorded variants with surface-dependent filtering and travel-based timing; source/license in `../THIRD_PARTY.md` |
| Spatial ambience without new libraries | Stereo panning and 3D distance attenuation for hum, distant sounds and door effects; generated ambience through existing WinMM output |
| Animated doors and wall-integrated exit | 0.8-second, 90-degree hinge movement; collision follows the panel, obstructed closing reverses, and escape requires walking through the open exit |
| Requested music without new libraries | User-supplied “Numbers” by Temporex, Windows WinMM/MCI playback, separate music toggle/volume and pause/resume without rewinding |
| Menus and settings | Main/pause menus and persistent camera, sound and display settings; music volume defaults to 35% and follows master volume |
| Optimization | Bounded streaming, GPU queue limits, cached sounds and a bounded voice pool; exact measurements in `verification.md` |
| Free exploration with optional clues | Flashlight clues, optional exit, local map and safe changes to unseen rooms retained |

The latest room-variety request supersedes the earlier one-room-type preference. The user reaffirmed that there should be no enemies. Furniture and separate themed districts remain removed. The default camera FOV eases between 78° and 82° during sprint travel; settings can change the base FOV. The exit occupies a wall opening centred at `(6.225, -87)`, approached from `(6.225, -85.8)`. E opens it; walking through the fully open doorway reaches the ending, and Enter resumes exploration. Pausing freezes the door animation and pauses the music.

Before submission: confirm team details, exact deadline, course template, instructor rubric and AI-use interpretation. Test the build on the TA's operating system. Record actual measurements and each student's contributions. The supplied syllabus says to submit via Moodle 2–3 days before the defence, according to the posted schedule. No submission has been made.

Use `verification.md` for checks executed against the current build. Earlier-version passes do not establish this version's behavior. Hidden functional captures differ from a visible FPS benchmark and a live keyboard/mouse playtest. Initial loading and cache misses remain synchronous.
