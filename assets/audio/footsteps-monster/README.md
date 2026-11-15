# Creature footstep fallback

Four softened sand contacts derived from Fantozzi's recordings, licensed CC0 1.0.
Source: https://opengameart.org/content/fantozzis-footsteps-grasssand-stone
License: https://creativecommons.org/publicdomain/zero/1.0/

Retained originals and decoded WAVs are in `../aranea/source/soil/`. Pitch,
spectral filtering, compression and short fades are authored in
`tools/prepare_aranea_audio.py`. These form the fallback bank when the main
four-kind creature bank is unavailable. No supplied heavy-footstep MP3 is used.
Rebuild with `python tools/prepare_release_audio.py`.
