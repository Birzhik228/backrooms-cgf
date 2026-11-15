# Creature contacts and calls

The runtime directory name is retained for compatibility. All current audio in
this bank derives from documented CC0 1.0 recordings.

- Walk/run: Fantozzi SandL1, SandR1, SandL2 and SandR2 from
  https://opengameart.org/content/fantozzis-footsteps-grasssand-stone
- Notice/roar: zerokingfull, Monster Roar 02, from
  https://freesound.org/people/zerokingfull/sounds/347410/
- License: https://creativecommons.org/publicdomain/zero/1.0/

The original Fantozzi FLACs and decoded WAVs remain under `source/soil/`. The
original roar OGG and its capture WAV remain under `../jumpscare/`. Calls use two
pitched/filtered layers of that CC0 recording; there are no YouTube excerpts.
Walk/run clips use filtered sand contacts. Four variants of each of the four
cue types load together before gameplay. `manifest.json` records inputs, hashes,
processing, sample counts and levels.

Rebuild: `python tools/prepare_aranea_audio.py` (NumPy, offline only).
`preview.wav` demonstrates the derived contacts and calls outside the game.
In-game positioning, walls, volume and leg-contact timing are applied separately.
