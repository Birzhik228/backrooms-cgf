# Submission build performance at 1280 by 720

Measured on 15 November 2026 using the clean-built Threshold Sentinel and CC0 audio release. These results identify the executable by SHA-256; the 14 November report belongs to the earlier build.

| AO setting | Median average FPS | Trial average range | Median p95 ms | Median p99 ms |
| --- | ---: | ---: | ---: | ---: |
| ON | 117.01 | 116.07-131.84 | 10.53 | 13.39 |
| OFF | 135.74 | 126.25-147.24 | 9.51 | 12.42 |

AO OFF increased the median trial-average FPS by 16.01%. Every trial averaged above 30 FPS in this fixed scene. Variation across trials is retained; these figures do not prove an uninterrupted minimum or every gameplay route.

## Individual measured trials

| Trial | Average FPS | Mean ms | p95 ms | p99 ms | Max ms | Frames over 33.333 ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| ao-on-1 | 116.07 | 8.62 | 10.53 | 13.39 | 46.46 | 1 |
| ao-off-1 | 135.74 | 7.37 | 9.51 | 12.42 | 37.04 | 2 |
| ao-off-2 | 147.24 | 6.79 | 8.46 | 9.77 | 31.20 | 0 |
| ao-on-2 | 131.84 | 7.59 | 8.91 | 9.85 | 14.67 | 0 |
| ao-on-3 | 117.01 | 8.55 | 11.89 | 23.02 | 56.33 | 12 |
| ao-off-3 | 126.25 | 7.92 | 12.98 | 16.24 | 35.55 | 1 |

## Method and computer

Three sequential trials per mode, reversing pair order on alternate repetitions. Each excludes at least five seconds of complete warmup frames and measures at least 30 subsequent seconds. FPS is 1000 divided by mean recorded frame time. Percentiles use nearest rank. No other build, native test or graphics test was run during these measurements. Ordinary desktop activity was not isolated.

The fixed live-rendered Level 0 presentation uses seed 12071998, x/z 3.75/3.75, yaw -90 degrees, pitch -5 degrees, FOV 78, exposure 1.1 and blur 0. AO ON is 0.85; OFF uses the actual G toggle and skips the AO pass. The scene and camera remain frozen, with identical geometry and no measured mesh uploads. VSync and the FPS cap are disabled, and the bounded GPU queue remains enabled.

Hidden mode suppresses audio output and avoids the unfocused-window cap. Frame timing includes CPU work, GPU queue waits and swap, but excludes small post-frame telemetry bookkeeping. It is not isolated GPU time. Screenshots, startup and shutdown are outside the measured windows.

CPU: 12th Gen Intel(R) Core(TM) i5-12450H. Renderer: Intel(R) UHD Graphics. OpenGL: 3.3.0 - Build 32.0.101.5768. RAM reported by Windows: 15.73 GiB. OS build: 26200. The installed NVIDIA adapter was not the renderer used in these trials.

Executable SHA-256: `48CAFA6D541B3D2C4D6EE5F52C049BBA1B9D5A12C8B62D9530E30CE460508303`.

All six runs finished with zero OpenGL errors and zero retained application-owned GL objects. Settings were unchanged. Raw logs, CSV files and original performance reports remain under the clean-build verification workspace; the JSON companion retains exact arguments, hardware, windows and metrics.

[Machine-readable comparison](ao-performance-2026-11-15.json). Reproduce with `tools/benchmark-ao.ps1` or run `Lab Check.cmd` from the extracted submission. The professor will use another computer; its performance has not been measured here.
