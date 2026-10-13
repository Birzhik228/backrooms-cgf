# Fantozzi sand contact source

- Author: **Fantozzi**; collection submitted to OpenGameArt by qubodup.
- License: **CC0 1.0 Universal**, as stated on the collection page.
- Collection: https://opengameart.org/content/fantozzis-footsteps-grasssand-stone
- Original pack: https://www.freesound.org/people/Fantozzi/packs/10338/
- Download: https://opengameart.org/sites/default/files/Fantozzi-footsteps.7z
- Retrieved: 13 October 2026.

Only `Fantozzi-SandL1`, `SandR1`, `SandL2`, and `SandR2` are used. Their original
FLAC files are retained here unchanged. Stone recordings are not included.
The adjacent WAVs were decoded with FFmpeg to mono PCM16 at 22050 Hz:

```sh
ffmpeg -i Fantozzi-SandL1.flac -ac 1 -ar 22050 -c:a pcm_s16le Fantozzi-SandL1.wav
```

Repeat for the other three names. `../../manifest.json` records all eight hashes.
`tools/prepare_aranea_audio.py` filters and adapts the WAVs into creature soil
footfalls. These are edited field recordings, not synthesized original sounds.
