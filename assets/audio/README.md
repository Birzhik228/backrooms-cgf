# Distributable audio bank

All 52 audio payloads are covered by `manifest.json`, with SHA-256, source author,
source URL, original/derivative status and CC0 1.0 license. The newly synthesized
music and pickup chime are original project work with disclosed Codex assistance
and are dedicated to CC0. This dedication does not change the named authors of
retained third-party recordings.

Rebuild from the repository root with NumPy installed:

```powershell
python tools/prepare_release_audio.py
```

The generator uses only retained CC0 recordings and authored oscillators/noise;
it does not download or extract video/audio references. Runtime needs no Python.
Older supplied media is kept only in the ignored local backup, outside the
submission package. Pixabay footsteps/Partygoer audio were replaced with CC0
sources to simplify source-package redistribution, not described as unlicensed.
