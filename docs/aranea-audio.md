# Aranea audio — 13 October 2026

Aranea uses separate banks for walking, running, sighting the player and
pursuit calls, with four variants per bank. Soil contacts use Fantozzi's CC0
sand recordings, softened and lowered in pitch. The hard reference impact and
delayed toe click were removed after the user's feedback. The vocal reference and
retained excerpts are documented in [asset provenance](../assets/audio/aranea/README.md).
Player audio, creature motion and the capture/jumpscare sample are preserved.

Footfalls still originate at ground height and fire only when one of the four
animated feet gains contact. The bank follows actual movement speed, with
hysteresis around the walk/run transition. A pair of diagonal running feet
plays two different variations. No ceiling rumble or free-running step loop
is added.

Detection calls use the creature's existing visibility result, including its
view angle, range and blocked line of sight. Hearing alone does not trigger a
sighting scream. Reacquisition is subject to a five-second cooldown and remains
queued while visible until eligible. Continuous sight triggers another call
every seven seconds; after key pickup, existing rage calls supply the periodic
vocals. A shared two-second vocal interval prevents overlapping calls.
Pause suspends timing and restarting clears the vocal state. Calls and both
step banks use existing world-space panning, distance attenuation and wall
transmission. Vocal near distance is 7 metres, while footsteps retain 3 metres.
Calls can replace the oldest lower-priority voice when all 12 slots are busy;
capture exclusivity, master volume and mute still apply.

The complete bank loads atomically before constructing the mixer. Missing or
malformed files discard the new bank and retain the existing safe fallbacks.
There is no per-step file read or new runtime decoder/library. The offline
rebuild script requires NumPy, while the game consumes ordinary PCM WAVs.

## Verification

All nine native suites passed for the initial bank with `build.ps1 -Test`.
The soil/call revision passed the expanded audio suite and a fresh game build.
The existing pause-order regression remains covered:
the old order drops a queued notice/roar in the first frame after Resume,
while synchronizing mixer pause before events retains its PCM. The correction
also preserves capture exclusivity and works without an audio device.

The 16 clips plus the offline preview rebuilt byte-identically from retained
inputs. Runtime clips have zero-valued endpoints, distinct variants and a
maximum absolute sample of 18677/32767. Mixer checks cover two simultaneous
running contacts plus a sighting call, panning, distance/wall attenuation,
mute, pause/reset, partial-bank failure and unchanged player/capture PCM.

Additional regressions cover a saturated voice pool, queued reacquisition and
continuous-visible calls. At master 0.75, detection RMS is 1708–1717 PCM units
at 14.32 m and 1078–1082 at 22 m. The former gain/distance settings applied to
the same new clip produce at most 632 and 388 respectively. This isolates the
audibility gain from the changed recording. Walls and facing still attenuate it.

`tests/audio_tests.exe --aranea-preview tests/artifacts/aranea-soil` exports the
actual mixer without normalization: walking (0–4 s), running (4–7 s), detection
at 14.32 m (7 s), and pursuit at 22 m (10 s). Separate detection previews are
also written. This makes the distant calls reviewable outside gameplay.

The final executable passed `tools/verify-aranea-audio.ps1` at 960x640:

| Scenario | Walk contacts | Run contacts | Sighting calls | Pursuit calls |
| --- | ---: | ---: | ---: | ---: |
| Walking, then encounter | 10 | 18 | 1 | 0 |
| Visible chase | 1 | 10 | 1 | 0 |
| Paused from startup | 0 | 0 | 0 | 0 |
| Key pickup | 0 | 32 | 1 | 1 |
| Capture and restart | 0 | 0 | 1 | 0 |

All five loaded the new bank and music, and reported zero OpenGL errors.
The last case completed one restart. The key fixture deliberately teleports
to the key; its synchronous chunk generation is not a travel-performance test.
Logs are retained locally in `tests/artifacts/aranea-audio/`. These are asset,
software-mixer and automated gameplay checks; they do not claim a physical
listening comparison or an exact match to the reference's perceived sound.
