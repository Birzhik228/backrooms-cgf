# Live demonstration: approximately 5 minutes

Demonstrate **BACKROOMS / Threshold v0.10**. Use WASD and the mouse, Ctrl to crouch, Shift to sprint, V for minimal HUD, Tab for the map, F for the flashlight, F1 for diagnostics, M to mute and E to open or close a nearby door. Rehearse a short route containing a door and contrasting floor surfaces, and explain only features you can defend.

| Time | Demonstration and explanation |
| --- | --- |
| 0:00–0:30 | Introduce the theme, team and disclosed AI assistance. Show the main menu and settings, including the separate music toggle and volume. Identify the supplied Temporex track. Explain that tension comes from the environment and there are no enemies. |
| 0:30–1:15 | Walk, stop, crouch and sprint briefly. Show smooth mouse look, weighted movement and the stamina bar. Compare recorded steps on rehearsed carpet/tile surfaces. |
| 1:15–2:00 | Show worn wallpaper under the flashlight and explain normal-mapped relief. Compare an active, dying or off fixture. Toggle F at the first doorway clue, then V to demonstrate light soft focus with a sharp HUD. |
| 2:00–3:00 | Open and close a nearby door, showing the 90-degree hinge animation. Pause briefly to freeze the panel, then resume. Show a corridor or hall and explain the larger 7.5-metre cells, six layout categories, reserved connected routes and 25 active chunks plus up to 24 prepared neighbours. |
| 3:00–4:00 | Follow the north route and use F1 and briefly F2. Explain triangles, UVs, camera matrices and attenuated lighting. Rotate near a humming fixture to demonstrate stereo position; describe environmental drone/stingers without claiming enemies. |
| 4:00–4:35 | Show the exit set into a wall opening. Press E, wait for the leaf to swing open, and walk through it to escape. Press Enter to continue. Explain that pressing E alone no longer triggers the ending and that the moving panel remains solid. |
| 4:35–5:00 | State measured results from `verification.md`, limitations and the student's own reviewed or authored changes. |

The starting position is `(3.75, 3.75)`. The clear north route follows `x = 3.75` toward the exit on the right. Approach `(6.225, -85.8)`, face north toward the wall opening at `(6.225, -87)`, and press E when prompted. Wait roughly 0.8 seconds for the door to open, then walk forward through it. At the normal walking pace, reaching this area takes roughly 40 seconds before time spent turning or demonstrating controls.

If navigation takes too long, use a rehearsed recording or clearly disclose a launch near the exit: `Backrooms.exe --demo --x 6.225 --z -85.8 --yaw -90`. This starts a demonstration near the landmark rather than proving a full traversal.

Audio backup: `Backrooms.exe --audio-preview assets\audio-preview.wav` generates a 12-second stereo demonstration without a graphics window. It includes recorded carpet/damp-carpet/tile footsteps, left/right ambience and environmental tension; it excludes the soundtrack and does not record a live playthrough. Keep the complete `assets/audio/` directory with the executable. Generated ambient sounds are placeholders, and positional playback uses stereo panning plus distance attenuation rather than wall-occluded acoustics. The supplied “Numbers” by Temporex is separate background music; pausing or muting pauses it without rewinding. Turn music off temporarily when comparing quiet footsteps.

Keep a screenshot and tested executable available. Use the current verification record for actual checks. Report FPS only with a measured scenario on the named machine.
