# Aranea ground movement and calls

Requested on 13 October 2026: creature walk/run contacts and a scream when it
spots the player, following the user's reference while keeping this creature
on the floor. Player footsteps and the existing capture scream are unchanged.

Reference: [Aranea Membri Sounds (Backrooms Entity)](https://www.youtube.com/watch?v=wzUCsTD23rg),
uploaded by **Slappy The Dummy**, 18 April 2025. Its description credits **@frag2**
for the entity/videos. This is the user's selected sound reference, not a claim
that the clips were extracted from a particular commercial game's assets.
The retrieved metadata does not state a reuse license. These are adapted
reference excerpts, not original recordings authored by this project.

The retained `source/running-reference.wav` covers 0.55–3.95 seconds;
`source/roar-reference.wav` covers 39.60–42.45 seconds. The latter passage is
labeled “ROARS” in the video. Source PCM is mono, 22050 Hz, 16-bit, decoded
offline from the video's audio. No ceiling-impact or eating passage is used.

Revised after the user reported hard-floor contacts and inaudible calls:
the four walk and four run contacts now use **Fantozzi's SandL1, SandR1, SandL2
and SandR2 recordings**, released as CC0. See [originals and conversion](source/soil/README.md).
Pitch reduction, low body and grit layers, compression and soft attacks produce
ground pressure and rustle. The old reference clank, shoe strike and delayed toe
click are absent. Running adds weight and shorter timing. Each clip represents
one planted foot. The historical running-reference excerpt is retained for
provenance but is no longer used by the current bank.

Four detection screams and four pursuit calls combine the roar excerpt with
the user's existing [ZeroKingFull recording](../jumpscare/README.md). Vocal EQ,
gentle compression and bounded levels bring the attack forward. Detection clips
last 1.84–1.915 seconds; pursuit clips last 1.54–1.615 seconds. The capture sample
itself is unchanged. All 16 runtime WAVs fit the existing bounded mono PCM
reader and are cached before gameplay.

`manifest.json` records exact source ranges, processing, hashes, sample counts,
peak and RMS values. Rebuild from the retained inputs with Python and NumPy:

```sh
python tools/prepare_aranea_audio.py
```

`preview.wav` demonstrates walk (0.35–3.40 s), paired running contacts
(4.00–5.67 s), detection (6.30–8.14 s), then pursuit (9.50–11.07 s).
This is an offline asset preview; in-game distance, walls, volume and the
actual four-leg contact timing are applied by the mixer and gameplay loop.
The audio test's `--aranea-preview DIR` option also writes an actual mixer
preview at master 0.75: walk near 6 m, run near 8 m, detection at 14.32 m and
pursuit at 22 m, without normalization.
