# Background soundtrack

Requested track: **Numbers — Temporex**. The user supplied `videoplayback.m4a` on 2026-10-05.

- `numbers-temporex.m4a` is an unchanged copy of that supplied file.
- `numbers-temporex.mp3` is the game-ready conversion, 192 kbps stereo, 44.1 kHz. Its duration is approximately 3 minutes 20 seconds.

The game plays the MP3 through its existing Windows WinMM library. There is no runtime download or additional audio library. The original M4A is retained for provenance; the MP3 is used because the installed Windows MCI driver could not open the original M4A codec/container combination.

Music loops during gameplay. The Settings music switch, music volume, master volume, pause menu, and M-key mute control it. Pausing or muting preserves the playback position. Missing or unplayable music is logged once and does not prevent the game from running.

This is a user-supplied commercial recording, separate from the project's CC0 footstep assets and generated environmental sounds. No alternate recording or generated imitation is substituted.

Conversion used FFmpeg 7.1, supplied by the local development-only `imageio-ffmpeg` package. Neither converter is required by the game. Reproduction command:

```text
ffmpeg -nostdin -i numbers-temporex.m4a -vn -map_metadata -1 -c:a libmp3lame -b:a 192k -ar 44100 -ac 2 numbers-temporex.mp3
```

Original M4A SHA-256: `49649496d359f944075ac5e81b3f730f388b20af4788f29066ddd3df8fb7298c`.

Native verification opened the MP3 successfully with a reported duration of 199734 ms, without starting playback. Separate native lifecycle tests use a generated silent WAV to verify pause, resume, mute, looping, and clean file/device release.
