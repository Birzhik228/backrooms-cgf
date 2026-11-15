# Monster Roar 02

Author: zerokingfull
Source: https://freesound.org/people/zerokingfull/sounds/347410/
License: CC0 1.0 Universal, verified on the source page on 15 November 2026.
License URL: https://creativecommons.org/publicdomain/zero/1.0/

The supplied file `347410__zerokingfull__monster-roar-02.ogg` is retained as
`monster-roar-02.ogg`. Its SHA-256 is
`512158b2c19311cebdbfaf8756d12acad6e8660b941da428987edc4b0d69bcda`.

`scream.wav` is the existing mono PCM16/22050 Hz derivative: initial silence
removed, trimmed to 1.95 seconds and given a 100 ms release. It remains unchanged.
The runtime caches pitch variants and adds a generated low impact; playback
respects mute/master volume. The same documented CC0 derivative also supplies
new creature calls and the Partygoer capture cue.

Offline conversion, using FFmpeg:

```text
ffmpeg -i monster-roar-02.ogg -af "silenceremove=start_periods=1:start_duration=0.005:start_threshold=-40dB,atrim=duration=1.95,afade=t=out:st=1.85:d=0.1" -ac 1 -ar 22050 -c:a pcm_s16le scream.wav
```

The global `../manifest.json` records both exact payload hashes. No decoder or
file access occurs at the capture trigger. A missing/invalid WAV is logged and
uses the generated fallback sting.
