# Jumpscare implementation

`common/jumpscare.h` contains `JumpscareManager`, `JumpscareState`, and the
`JUMPSCARE` defaults. The states are Inactive → Lunge (0.22 s) → Hold (until
2.0 s) → End. End displays pure black for 0.30 s before the existing Game Over
menu. Escape skips the presentation into the same black interval. Restart
resets the manager, player, doors, creature, audio ownership, and shader values.

An active creature within 1.5 m and an unobstructed world/door sight segment
triggers the scare, as does an existing catch. J triggers a debug scare,
including from the pause menu; if necessary it places the actor nearby.
Repeated triggers cannot restart a live scare or replay its sound.

Movement and mouse look lock through the existing pause/input path. AI stops.
The camera turns toward the eye anchor with cubic ease-out, uses a 70–95 degree
FOV punch and small oscillation, and gets decaying pseudorandom translation and
roll sampled at 85 Hz. The animated actor lunges to 0.75 m, scales for the
close-up, and faces the camera in yaw and pitch. It uses the supplied rig's
scream clip, including its articulated arms; no substitute sprite is used.

The existing scene FBO and fullscreen triangle provide RGB separation, static,
pulsing vignette, late red tint, and exactly two rendered frames of white flash.
All additional effects have switches in `JumpscareConfig`. Their uniforms are
set every frame, including zeros outside the scare. The existing framebuffer
resize/lifetime handling remains in charge; no extra framebuffer is allocated.

The mesh already tags eyes and teeth with material 21. That material is the
emission mask, so no brightness/UV guess is necessary. During the scare its
emission increases while other lighting/emission darkens. A 12-tap bright-pass
blur in the existing lens pass adds a cheap glow; it costs samples only during
the scare. This is LDR glow rather than a separate HDR bloom chain and can also
catch very bright scene pixels. The face remains the dominant emitter.

The supplied roar is preloaded and pitch variants plus a low impact are cached.
At the trigger, music pauses, queued waveOut buffers are flushed, other voices
are cleared, and the first roar buffer is submitted immediately without a
fade-in. Hum, drone, ambience scheduling, and other one-shots remain suppressed.
Hardware/output-device latency still applies. Audio stops at End; master volume
and M mute remain effective. Audio provenance is in
`assets/audio/jumpscare/README.md`.

## Checks

Run `build.ps1 -Test` and `tools/verify-jumpscare.ps1`. CPU tests cover state
timing at 30/60/144 Hz, invalid delta times, focus suspension, one-shot trigger,
distance/visibility conditions, two-frame flash, black delay, and repeat reset.
Audio tests cover the supplied file, immediate impact, exclusive audio,
headroom, silence after the roar, and reset. Existing movement, world, streaming,
creature, model, and music suites remain in the build.

Hidden OpenGL captures exercise the J callback, catch, close-up, flash, red tint,
black interval, Game Over, and repeated scare after restart. These are automated
checks, not a physical keyboard/mouse or speaker-listening playtest.

Verified on 11 October 2026 with MSYS2 UCRT64 GCC 14.2 and Intel UHD OpenGL
3.3. All eight build suites passed, including the extended state tests. The
additional supplied-roar audio checks passed. All ten graphics cases passed
with zero GL errors; black/flash pixels were checked exactly. Close-up, late
tint, black and Game Over captures were visually inspected. Evidence is in
`tests/artifacts/jumpscare/` (ignored generated files).
