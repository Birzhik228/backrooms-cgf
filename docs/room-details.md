# Ventilation covers, exit light, doors, controls and mouse

Vent pockets retain their existing seeded positions and dimensions. Their mouth
now has a metal frame, ten horizontal slats and a centre support, using the same
aged metal material as door hardware. The gaps are geometry, so both sides have
an actual view. The closed cover blocks walking; E-entry and E-exit represent
lifting and replacing it, placing the player safely on the other side without
an opening animation. Existing line-of-sight restrictions on hiding still apply.

The exit has a neutral white ceiling panel and two white light sources. The
green exit sign remains readable. Door material 17 uses procedural brown grain
in the moving leaf's UV coordinates, keeping it attached through the hinge swing.
No texture download or new renderer dependency is required.

The controls guide appears on entering a normal run and each fresh restart.
Three pages teach movement, exploration and survival, with keycap drawings and
native vector illustrations of controls, flashlight, key, door, vent and mouse.
Captions use separate rows rather than slash-delimited instructions. Layout and
button hit areas share a scale transform for smaller windows and high DPI.
Next, Enter and right arrow advance; Back and left arrow return. The last page
offers Begin Exploring. Escape, H and the top-right close button dismiss it at
any stage. H reopens at page one during exploration. It pauses simulation and
sound, frees the cursor, and consumes input so navigation cannot also trigger a
menu item. Hidden automated runs skip it unless --guide-check is requested.
Static movement instructions are removed from the HUD and pause menu;
situational prompts and status remain.

`common/mouse.h` contains one small resident mouse, built from low-poly curved
body, head, ears, paws and tail pieces. It wanders with collision checks and
scurries away on contact. It stays in nearby streamed space, resets with the
player, and freezes in menus, the popup and capture. Contact within 0.48 m,
without an intervening wall, triggers two short synthesized positional chirps.
There is a three-second cooldown plus a leave-contact requirement, preventing
continuous squeak spam. Hidden players do not trigger it.

The mouse also breathes, sniffs, twitches its ears, lifts alternating paws and
swishes its tail. Locomotion follows actual travel; small idle motions use delta
time. All animation freezes with gameplay. A quiet positional chirp and rustle
starts after 1.8 seconds, then repeats at seeded 3.5–6 second intervals. These
discovery cues are audible nearby, attenuated by walls and distance, and never
alert the creature. Contact suppresses the quiet cue while the mouse flees.

The creature remembers the squeak location and searches there using its existing
navigation. The cue can end spawn grace, but spawning still requires an unseen,
reachable location. If no valid spawn exists immediately, the alarm is retained
briefly for a retry. An already active creature only hears the cue within 48 m,
reduced by the square root of wall transmission. Subsequent sight and movement
noise continue to use ordinary pursuit logic.

`build.ps1 -Test` covers vent blocking and usable interiors, white exit lights,
mouse contact/rearming/pause/reset, geometry, creature alarm response and audible
mouse PCM through the mixer. `tools/verify-room-details.ps1` captures the popup,
keyboard/click dismissal, mouse contact/model, wooden door, white-lit exit and
both sides of the ventilation cover. Evidence is written under
`tests/artifacts/room-details/`. These are automated checks, not a human playtest.

Verified on 11 October 2026: all nine native suites, ten progression render
cases and nine room-detail render cases passed with zero OpenGL errors.
After refining the mouse tail mesh, the progression suite and mouse preview
were rerun successfully. Screenshots of the grille view, popup, wooden door,
white-lit exit and complete mouse were inspected. Audio was checked through
the PCM mixer; physical speaker listening was not performed.

The animation and passive-audio follow-up passed the progression and audio
suites, including paused poses, above-floor animation, quiet-cue cadence and
playback through the mixer. Mouse contact and preview rendered checks also
passed. Quiet cues remain separate from contact alarms.

The illustrated guide revision passed eight rendered checks: all three pages,
back navigation, Escape dismissal, keyboard completion, mouse completion and
the 960-by-640 layout. Page captures were visually inspected for readability
and clipping; every run reported zero OpenGL errors.
