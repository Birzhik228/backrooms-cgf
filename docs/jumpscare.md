# Jumpscare implementation

`common/jumpscare.h` contains `JumpscareManager` and its validated configuration.
The default presentation lasts **2.0 seconds**: Inactive → Lunge (0.22 s) →
Hold → End. End displays pure black for 0.30 s before Game Over. Escape skips
the presentation into the same black interval. Restart clears capture, player,
doors, creature, audio ownership and effects. Repeated triggers cannot restart
an active scare or replay its sound.

An active creature within 1.5 m and an unobstructed world/door sight segment
triggers the scare, as does an existing catch. **J** triggers a debug scare,
including from the pause menu; if necessary it places the actor nearby.

## Camera and creature

At capture, the game derives a head target from the current animated mesh and
transforms it using the creature's position and facing. It freezes player
movement and mouse look, then turns toward that target over **0.20 seconds**
with cubic ease-out. Yaw takes the shortest arc. The camera turn has its own
duration, independent of the creature's 0.22-second lunge.

The attack remains along the captured head direction while the camera turns.
The creature's cinematic eye anchor advances from 1.65 m to **0.90 m** from the
stable camera position; yaw and pitch make the face look back at the camera,
including when the player was crouching. Scaling around the eye anchor grows
from 1× to **2.2×** with the lunge, giving a large head close-up without moving
that anchor farther away. The default scale keeps the jaw visible and the snout
clear of the camera's near plane. The camera's random shake remains independent
of the stable placement.

The supplied Aranea GLB has no animation. The project therefore authors a
capture clip on its mesh-part rig: the body thrusts forward, the front claws
lift and reach, the jaw opens, and small head yaw/pitch jitter adds motion.
The head counter-rotates against the body's forward pitch, keeping its face
aimed at the camera with approximately ±0.025 radians of pitch jitter.
The clip is sampled by normalized capture progress, so changing the total
duration changes its playback speed. The original source is unchanged. See
[Aranea model notes](../assets/models/aranea/README.md).

Camera shake uses four independently salted integer-hash channels sampled at
**90 Hz**: three position offsets and a small roll. Its amplitude decays to
zero across the presentation. The FOV punches from **70° to 95°** over the
first 0.12 seconds, then has a small decaying oscillation and settles slightly.
Pausing due to focus suspension freezes the elapsed time and effect samples.
The black interval clears shake; ordinary gameplay uses its normal camera FOV.

## Configuration

Edit **`jumpscare.cfg`** beside the executable, then restart the game. An
alternative file can be selected with:

```powershell
.\Backrooms.exe --jumpscare-config .\my-jumpscare.cfg
```

Each line is `key = value`; `#` starts a comment. Missing files use built-in
defaults. Unknown keys, malformed values and nonfinite numbers are ignored;
finite values outside supported ranges are clamped.

| Setting | Default | Supported range / units |
| --- | ---: | --- |
| `duration` | 2.0 | 0.5–6 seconds, excluding the black delay |
| `camera_turn_duration` | 0.20 | 0.15–0.25 seconds |
| `lunge_duration` | 0.22 | 0.10–0.50 seconds |
| `face_distance` | 0.90 | 0.6–1.0 metres to the eye anchor |
| `closeup_scale` | 2.2 | 1–6 times the model scale |
| `shake_offset` | 0.026 | 0–0.15 metres per axis before decay |
| `shake_roll` | 0.025 | 0–0.12 radians before decay |
| `fov_start` | 70 | 50–110 degrees |
| `fov_peak` | 95 | From `fov_start` to 120 degrees |
| `black_delay` | 0.30 | 0–2 seconds |
| `trigger_distance` | 1.5 | 0.5–3 metres, still requiring line of sight |

Boolean keys accept `true`/`false` or `1`/`0`: `enabled`, `camera_turn`, `lunge`,
`shake`, `fov_punch`, `chromatic`, `noise`, `vignette`, `red_tint`, `flash`,
`glow`, `darken`, and `audio`. All default to enabled. Additional postprocess
strengths are defined in `JumpscareConfig` in the header.

## Rendering and audio

The existing scene framebuffer and fullscreen triangle provide RGB separation,
static, pulsing vignette, late red tint and exactly two rendered frames of white
flash. Uniforms reset outside the scare. Framebuffer lifetime and resizing use
the normal rendering path; the scare allocates no additional framebuffer.
Aranea retains its supplied materials and receives a capture face fill. The
earlier Smiler's permanent glowing eye/teeth material is not applied to Aranea.
The lens pass can add a 12-tap bright-pass glow during capture.

The supplied roar is preloaded, with pitch variants and a cached low impact.
At the trigger, music pauses, queued waveOut buffers are flushed, other voices
are cleared, and the first roar buffer is submitted immediately without a
fade-in. Hum, drone, ambience and other one-shots remain suppressed. Hardware
and output-device latency still apply. Audio stops at End; master volume and
**M** mute remain effective. See [audio provenance](../assets/audio/jumpscare/README.md).

## Validation — 12 October 2026

The nine native suites passed; after final close-up tuning, model and encounter
checks were rerun successfully. Actual orbital landmarks verify that the
neutral skull faces forward and all authored/interpolated capture frames keep
their eye anchor within 1 mm. The previous sideways bank fails this regression.

The final executable passed all 10 `tools/verify-jumpscare.ps1` cases and all
15 `tools/verify-aranea.ps1` cases, with zero GL errors. These cover flash/black
pixels, standing and crouched close-ups, proximity, game over, repeat trigger,
restart, walk/run poses, door pursuit, pause and an odd framebuffer size.

Six additional `tools/verify-forward-scare.ps1` cases measure the actual camera
and body transforms through frame profiles. Side and rear captures turn about
90° and 180° and finish at 0.20 s; the lunge settles at 0.90 m at 0.22 s; FOV
reaches about 95°. Late shake energy is under 0.23% of early shake energy.
A 3-second configuration remains active after 2.6 s and reaches game over at
3.3 s including the black interval. Moving body direction agrees with travel
with dot product above 0.99998, and the head stays centered during the gait.

See [machine-readable results](forward-jumpscare-results.json). The initial
3.2× close-up was reduced to 2.2× after inspecting the snout and jaw framing.
Automated captures were visually inspected; this was not a physical
keyboard/mouse or speaker-listening playtest.
