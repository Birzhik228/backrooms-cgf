# Threshold Ambient

Original 48-second stereo ambient composition generated specifically for this
project by `tools/prepare_release_audio.py`, with disclosed Codex assistance.
Slow oscillator layers, filtered seeded noise and sparse bell-like partials
create an electrical interior atmosphere. No sampled music, lyrics, transcribed
melody or reference-track imitation is used.

`threshold-ambient.wav` is 22050 Hz stereo PCM16, with 2.5-second entry/exit fades.
The existing Windows MCI player loops it and retains the music toggle, separate
music volume, master volume, mute and pause/resume behavior. There is no runtime
converter or network request. `manifest.json` contains its exact hash and levels.

The newly synthesized audio is dedicated to CC0 1.0 Universal:
https://creativecommons.org/publicdomain/zero/1.0/
This dedication applies only to the new generated composition. Rebuild with
`python tools/prepare_release_audio.py` (NumPy, offline).
