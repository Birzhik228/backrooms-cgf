# Supplied jumpscare roar

The user supplied `347410__zerokingfull__monster-roar-02.ogg` on 11 October 2026.
`monster-roar-02.ogg` preserves that file unchanged. Its filename identifies
ZeroKingFull and “monster roar 02”; no license metadata was supplied or verified.

Original SHA-256: `512158b2c19311cebdbfaf8756d12acad6e8660b941da428987edc4b0d69bcda`.

`scream.wav` is a 22050 Hz mono PCM16 derivative, with initial silence removed,
trimmed to 1.95 seconds, and a 100 ms release. There is no added fade-in.
Conversion (FFmpeg is a development tool, not a runtime dependency):

```sh
ffmpeg -i monster-roar-02.ogg -af "silenceremove=start_periods=1:start_duration=0.005:start_threshold=-40dB,atrim=duration=1.95,afade=t=out:st=1.85:d=0.1" -ac 1 -ar 22050 -c:a pcm_s16le scream.wav
```

The mixer caches four resampled pitch variants (0.96–1.038 times source pitch),
adds a decaying low impact, and selects one using its seeded random generator.
The roar respects master volume and mute. Missing/invalid WAV uses the existing
synthesized capture sting and logs the fallback. No decoding or file access
occurs at the trigger.
