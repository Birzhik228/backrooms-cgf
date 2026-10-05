"""Prepare the six CC0 recordings for the game (offline only).

Requires numpy and soundfile, not used by the built executable.
Source recordings stay unchanged in assets/audio/footsteps/source.
The generated manifest records original and derived SHA-256 hashes.
"""
from pathlib import Path
import hashlib
import json
import math
import wave

import numpy as np
import soundfile as sf

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "assets/audio/footsteps"
SOURCE_URL = "https://opengameart.org/content/footsteps-0"
BASE_URL = "https://opengameart.org/sites/default/files/"
NAMES = ["01-footstep_0.ogg"] + [f"{n:02}-footstep.ogg" for n in range(2, 7)]
RATE = 22050


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def prepare(source, target):
    samples, source_rate = sf.read(source, dtype="float64", always_2d=True)
    samples = samples.mean(axis=1)
    if not np.isfinite(samples).all() or len(samples) < 100:
        raise ValueError(f"Invalid recording: {source}")
    # Remove only sub-bass/DC; keep the recorded heel, shoe scrape and room tail.
    high = np.zeros_like(samples)
    alpha = math.exp(-2 * math.pi * 35 / source_rate)
    previous_input = previous_output = 0.0
    for index, value in enumerate(samples):
        previous_output = alpha * (previous_output + value - previous_input)
        high[index] = previous_output
        previous_input = value
    # Windowed-sinc low-pass before downsampling: soft indoor shoe contact and
    # anti-aliasing, without adding any generated thump or artificial reverb.
    taps = np.arange(-64, 65, dtype=np.float64)
    cutoff = 3600 / source_rate
    kernel = 2 * cutoff * np.sinc(2 * cutoff * taps) * np.hamming(len(taps))
    kernel /= kernel.sum()
    filtered = np.convolve(high, kernel, mode="same")
    positions = np.arange(math.ceil(len(filtered) * RATE / source_rate)) * source_rate / RATE
    converted = np.interp(positions, np.arange(len(filtered)), filtered)
    # Preserve the full recorded contact. Tiny fades prevent boundary clicks.
    attack, release = int(.004 * RATE), int(.018 * RATE)
    converted[:attack] *= np.linspace(0, 1, attack)
    converted[-release:] *= np.linspace(1, 0, release)
    converted *= .88 / np.max(np.abs(converted))
    converted = np.pad(converted, (0, int(.035 * RATE)))
    pcm = np.rint(converted * 32767).astype("<i2")
    pcm[0] = pcm[-1] = 0
    with wave.open(str(target), "wb") as output:
        output.setnchannels(1)
        output.setsampwidth(2)
        output.setframerate(RATE)
        output.writeframes(pcm.tobytes())
    return {
        "source": source.name,
        "source_url": BASE_URL + source.name,
        "source_sha256": sha(source),
        "source_sample_rate": source_rate,
        "output": target.name,
        "sha256": sha(target),
        "samples": len(pcm),
        "seconds": round(len(pcm) / RATE, 6),
        "peak": int(np.max(np.abs(pcm))),
        "rms": round(float(np.sqrt(np.mean(pcm.astype(float) ** 2))), 3),
    }


def main():
    manifest = {
        "title": "Footsteps",
        "author": "GboxMikeFozzy",
        "source_page": SOURCE_URL,
        "license": "CC0-1.0",
        "license_url": "https://creativecommons.org/publicdomain/zero/1.0/",
        "source_verified": "2026-10-03",
        "description": "Six real shoe footfalls recorded by the author while walking through a subway.",
        "processing": "35 Hz high-pass, 3.6 kHz low-pass, resample 22050 Hz mono, short edge fades, normalize peak to 0.88, 35 ms silent tail. No synthetic impact layer.",
        "clips": [],
    }
    for number, name in enumerate(NAMES, 1):
        manifest["clips"].append(prepare(OUT / "source" / name, OUT / f"step-{number:02}.wav"))
    (OUT / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    main()
