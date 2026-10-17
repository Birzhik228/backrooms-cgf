# Lighting and restrained material update

11 October 2026; source baseline `6bac66888650253b1f8fea9b7643061ff20f0a90`.

The flashlight now casts shadows from architecture, moving doors, ventilation
slats and the imported animated creature. A 1024px depth map and nine-tap PCF
filter keep edges restrained. The emitter is offset 16cm right and 10cm down,
with a collision-aware return to the camera position. Capture presentation
disables these ordinary flashlight shadows to retain the existing close-up.

Up to 24 selected ceiling lights use six 256px depth layers each, on the
existing OpenGL 3.3 baseline. Full rendered geometry blocks direct light,
including live door panels and actual gaps between vent slats. Maps are cached
against the lamp position, nearby chunk revisions and nearby door animation.
Lamp flicker only changes light power. Small ambient fill remains; this is
direct shadowed lighting, not global illumination. The creature casts a moving
flashlight shadow; it is not included in the static ceiling-light cache.

Contact Shading uses a 12-sample screen-space hemisphere at half resolution,
depth-derived normals and depth-aware upsampling in the lens pass. Strength
defaults to 65% and can be reduced to zero in Settings, which skips the AO pass.
It freezes with the scene in menus and is disabled for capture presentation and
wireframe. Like other screen-space effects it cannot see off-screen occluders.
It mildly modulates the final scene rather than supplying physically based GI.

Paper now has small tone and normal variations; carpet varies slightly in wear
and normal strength, with rare damp specular patches; ceiling panels have mild
age variation. Existing geometry, yellow palette, floral paper, marks and
texture banks are retained. The new wear functions repeat over the render
origin's 180m phase, so origin rebasing does not move the material. Wet patches
are lighting highlights, not reflected scene images.

## Validation

- Release build uses GCC C++17, O2, Wall/Wextra.
- All nine existing native suites passed. Windows music lifecycle was rerun
  outside the sandbox because restricted Windows MCI initialization returned
  error 277; that unrestricted rerun passed.
- New rendered probes check all six lamp directions against door, metal and
  creature blockers, plus transparent clues: 24 cases. Four flashlight probes
  cover the same materials. AO tests assert no darkening on an isolated flat
  wall and actual darkening at a floor/wall contact.
- Settings checks cover contact shading persistence, old-file defaults and
  invalid strength clamping.
- Fourteen room-detail/guide cases, ten progression cases and ten jumpscare
  cases passed with zero GL errors. The existing jumpscare restart assertion
  was corrected to check separate state fields: newer mouse/guide log fields
  had invalidated its contiguous-text assumption.
- Additional captures cover the 960x640 settings panel, creature shadow,
  animated door and travel through a render-origin phase boundary. Vent views,
  exit lighting, creature and settings captures were visually inspected.

## Local performance sample

Intel UHD Graphics, 1280x800, hidden, VSync off, 240-frame scripted walking,
no creature, same seed and route. The first 60 frames were excluded. Old and
new builds were run separately; these are local frame timings, not GPU-only
timers or a claim about performance on other computers.

| Frame time | Previous graphics | Updated graphics |
| --- | ---: | ---: |
| Median | 6.69ms | 13.12ms |
| 95th percentile | 10.75ms | 17.69ms |

This short run includes a later synchronous streaming/room-change outlier:
maximum 643.11ms before and 723.12ms after. Both runs reported zero streaming
fallback chunks. The new run refreshed 197 lamp caches during traversal and
room changes. Existing synchronous generation remains a source of hitches;
this change is visual work, not a streaming optimization.

Shadow resolutions are fixed. Depth maps use approximately 40MB if the driver
stores depth24 textures in 32 bits, plus the screen depth and half-size AO target.
Low resolution point shadows may have small seam/edge errors; shadow bias can
slightly detach contact edges. Contact Shading can be disabled for lower cost.

Run `build.ps1 -Test -GraphicsTest` on Windows for native suites and GL probes.
CMake exposes optional `BACKROOMS_GRAPHICS_TESTS`; alternate build paths have
not been executed in this update. No physical audio listening or human gameplay
session is claimed by automated checks.

Final 1200-frame sprint/streaming smoke: 25 resident plus 24 prepared chunks,
10 cache promotions, zero synchronous fallback chunks and zero GL errors.
AO-disabled rendering also passed. Installed-copy smoke passed outside the
sandbox with music loaded and zero GL errors.

Installed into the canonical Desktop project with originals preserved at
`backups/before-graphics-20261011-223309`. Settings and assets were preserved;
source changes remain local and have not been committed or pushed.

## Camera-grain grid correction

A user screenshot revealed a fine screen-wide mesh. Isolating the effects
showed that disabling AO did not remove it, while disabling camera grain did.
The original sine-based floating-point noise lost spatial independence as
elapsed time increased. A uniform-image rendering test on the same Intel UHD
reproduced a maximum short-lag correlation of 0.959914 at 60 seconds.

The lens shader now hashes integer pixel coordinates and a bounded frame seed,
with independent seeds for ordinary grain and jumpscare static. Strength and
24Hz/90Hz update cadence are retained. The same test checks distribution,
short-lag correlation, column/row banding, frozen/advanced time and full white/
black capture frames. It is included in `build.ps1 -GraphicsTest` and the optional
CMake graphics tests. The corrected shader also passed a 1920x1000 game capture
with zero GL errors. AO, shadowing and material shaders were not changed for
this correction.

The corrected noise passed at 0, 60, 300, 1200, 3600, 10000 and 30000 seconds,
including a 1919x1027 odd-sized target. Maximum short-lag correlations remained
below 0.0017 for ordinary grain and 0.0025 for scare static; the original 60-second case failed the
same test. In-game black and white jumpscare frames also passed exactly.

## Wall-band correction â€” 12 October 2026

A subsequent screenshot showed vertical bands on walls viewed at an angle.
Controlled captures with flat wallpaper color and normals retained the bands;
disabling SSAO removed them. The half-resolution AO pass sampled depth between
full-resolution pixel centers, then reconstructed positions at mismatched
coordinates. This produced false surface normals and self-occlusion. The lens
pass also lacked spatial interpolation weights when enlarging AO.

AO now reads exact depth texels and reconstructs positions at those same pixel
centers. Samples on the receiving plane cannot occlude it. The lens combines
bilinear spatial weights with depth rejection and handles shortened final
intervals for odd framebuffer dimensions. Wallpaper, materials, grain strength
and the Contact Shading setting are preserved.

The rendered regression suite checks 24 analytic tilted and grazing planes at
1920x1000, 1920x1027, 1919x1027 and 127x95. All 4,885,962 checked unoccluded
pixels returned AO 255/255. The original shader fails the first case with a
minimum of 201/255. The original lens reconstruction independently fails the
smooth-gradient test. The fixed shaders pass that test, depth-edge rejection,
real floor/wall contact darkening and all 28 existing shadow probes.

Game captures near the reported wall at 1920x1000 and 1919x1027, plus a close
1280x800 view, rendered without GL errors. Visual comparison confirmed removal
of the repeated dark wall bands while retaining the existing wallpaper design.
Only external GLSL shaders and regression/documentation files changed; the
game executable does not require rebuilding.


## Exit performance and FPS display — 17 October 2026

A 240-frame 1280x800 exit-door fixture rebuilt lamp shadows 456 times as the
leaf animated. Door angles are now quantized into eight shadow-cache steps,
with at most two already-initialized lamp maps refreshed per frame. New lamp
maps initialize immediately; stationary door endpoints converge over subsequent
frames. This can briefly delay a lamp shadow during motion; physical collision
and the visible door still animate every frame. The signed opening direction
is part of the cache revision. The near-black exit lining now uses a cheap
light-absorbing shader path instead of evaluating 24 lamps and flashlight PCF.

Excluding the first 40 frames, the same fixture improved from 27.3 to 31.1 FPS,
with 95th-percentile frame time falling from 49 to 35 ms and cache rebuilds from
456 to 136. A stationary closed-door comparison measured 33.1 to 36.2 FPS;
these short local Intel UHD measurements do not establish a cure for every
exit/transition stall. Initial generation and new shadow-map initialization
still have a cost. Profiles are in `tests/artifacts/exit-performance/`.

Settings now shows live FPS and has a persistent FPS COUNTER toggle for the
in-game overlay. It defaults off for existing configurations. The 13 settings
rows fit at 960x640 and keyboard/mouse selection share the same 27-pixel spacing.
Settings persistence, both door opening directions, streaming of signed hinge
poses, and creature pursuit are covered by native tests and rendered checks.
