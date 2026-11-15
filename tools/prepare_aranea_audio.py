"""Rebuild creature contacts and calls using only the retained CC0 recordings.

Offline dependency: NumPy. Runtime uses only the resulting PCM16 WAV files.
"""
import argparse
import hashlib
import json
import wave
from pathlib import Path

import numpy as np

RATE = 22050
ROOT = Path(__file__).resolve().parents[1]
BANK = ROOT / 'assets/audio/aranea'


def read(path):
    with wave.open(str(path), 'rb') as stream:
        assert (stream.getnchannels(), stream.getsampwidth(), stream.getframerate()) == (1, 2, RATE)
        return np.frombuffer(stream.readframes(stream.getnframes()), dtype='<i2').astype(np.float64) / 32768


def write(path, samples):
    path.parent.mkdir(parents=True, exist_ok=True)
    pcm = np.rint(np.clip(samples, -.94, .94) * 32767).astype('<i2')
    with wave.open(str(path), 'wb') as stream:
        stream.setparams((1, 2, RATE, 0, 'NONE', 'not compressed'))
        stream.writeframes(pcm.tobytes())
    return {'file': path.relative_to(BANK).as_posix(), 'samples': len(pcm),
            'seconds': round(len(pcm) / RATE, 6), 'peak': int(np.max(np.abs(pcm.astype(int)))),
            'rms': round(float(np.sqrt(np.mean((pcm.astype(float) / 32768) ** 2))), 6),
            'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}


def band(samples, low, high):
    # Zero padding prevents wrap-around; broad slopes avoid metallic FFT ringing.
    size = 1 << (len(samples) * 2 - 1).bit_length()
    frequency = np.fft.rfftfreq(size, 1 / RATE)
    high_pass = 1 - np.exp(-(frequency / low) ** 4)
    low_pass = np.exp(-(frequency / high) ** 4)
    return np.fft.irfft(np.fft.rfft(samples, size) * high_pass * low_pass, size)[:len(samples)]


def pitched(samples, factor, count):
    position = np.arange(count) * factor
    return np.interp(position, np.arange(len(samples)), samples, left=0, right=0)


def edges(samples, attack=.003, release=.065):
    time = np.arange(len(samples)) / RATE
    fade_in = np.sin(np.minimum(time / attack, 1) * np.pi / 2) ** 2
    fade_out = np.sin(np.minimum((len(samples) - 1 - np.arange(len(samples))) / RATE / release, 1) * np.pi / 2) ** 2
    return samples * fade_in * fade_out


def normalize(samples, rms, peak):
    samples = samples - np.mean(samples)
    samples = edges(samples)
    level = np.sqrt(np.mean(samples ** 2))
    samples *= min(rms / max(level, 1e-9), peak / max(np.max(np.abs(samples)), 1e-9))
    samples[0] = samples[-1] = 0
    return samples


SOIL_NAMES = ['SandL1', 'SandR1', 'SandL2', 'SandR2']


def make_steps(running):
    result = []
    for variant, name in enumerate(SOIL_NAMES):
        duration = (.34 if running else .41) + .008 * variant
        count = round(duration * RATE)
        time = np.arange(count) / RATE
        source = read(BANK / f'source/soil/Fantozzi-{name}.wav')
        # Remove recording lead-in without selecting a sharp isolated click.
        envelope = np.convolve(np.abs(source), np.ones(110)/110, mode='same')
        active = np.flatnonzero(envelope > np.max(envelope) * .07)
        begin = max(0, int(active[0]) - round(.010*RATE))
        source = source[begin:]
        source = pitched(source, (.90 if running else .79) + .015*variant, count)

        # Outdoor sand/grain compression, no hard-floor strike, reference
        # clank, delayed toe click or artificial reverberation.
        body = band(source, 40, 650)
        body /= max(np.max(np.abs(body)), 1e-6)
        grit = band(source, 250, 1900 if running else 1500)
        grit /= max(np.max(np.abs(grit)), 1e-6)
        contact = .70 * body + .28 * grit
        # Bring up the low crunch between peaks rather than making the initial
        # strike louder. Filter after compression to keep the contact dull.
        contact /= max(np.max(np.abs(contact)), 1e-6)
        contact = band(np.tanh(contact*2.7), 38, 1900 if running else 1550)
        # Soft onset and broad pressure envelope feel like weight entering
        # yielding ground. Run gains weight, not a brighter ceramic attack.
        contact *= np.exp(-time / (.23 if running else .29))
        contact = edges(contact, .011 if running else .018, .10)
        sample = normalize(contact, .12 if running else .085, .57 if running else .43)
        result.append(sample)
    return result


def make_calls(notice):
    result = []
    scream = band(read(ROOT / 'assets/audio/jumpscare/scream.wav'), 90, 6200)
    for variant in range(4):
        duration = (1.84 if notice else 1.54) + .025 * variant
        count = round(duration * RATE)
        # Both layers come from zerokingfull's CC0 Monster Roar 02 recording.
        # No YouTube excerpt, commercial recording or reference imitation.
        source = scream[round((.015 + .022*variant) * RATE):]
        attack_voice = pitched(source, (1.12 if notice else .91) + .02*variant, count)
        body_voice = band(pitched(source, (.72 if notice else .61) + .018*variant, count), 75, 2200)
        voice = .80*attack_voice + .28*body_voice
        voice /= max(np.max(np.abs(voice)), 1e-6)
        # A clear vocal attack carries over distant footsteps/music. Gentle
        # compression retains its quieter throat body; no change to capture PCM.
        voice = band(np.tanh(voice*2.0), 110 if notice else 75, 5600 if notice else 3600)
        voice = edges(voice, .010 if notice else .028, .22)
        sample = normalize(voice, .27 if notice else .23, .84 if notice else .76)
        result.append(sample)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.parse_args()
    clips = {'walk': make_steps(False), 'run': make_steps(True),
             'notice': make_calls(True), 'roar': make_calls(False)}
    files = []
    for kind, bank in clips.items():
        for index, clip in enumerate(bank):
            files.append(write(BANK / f'{kind}-0{index+1}.wav', clip))

    # Walk, diagonal ground run, sighting scream, quieter pursuit call.
    preview = np.zeros(round(13.5*RATE))
    def add(when, clip, gain=1):
        at=round(when*RATE);preview[at:at+len(clip)] += clip*gain
    for i in range(8):add(.35+i*.37, clips['walk'][i%4], .8)
    for i in range(5):
        add(4.0+i*.32, clips['run'][(2*i)%4], .55)
        add(4.0+i*.32+.025, clips['run'][(2*i+1)%4], .55)
    add(6.3, clips['notice'][0], .9)
    add(9.5, clips['roar'][1], .85)
    write(BANK/'preview.wav', preview)
    manifest = {'sample_rate': RATE, 'format': 'mono PCM16', 'call_source': {
        'url': 'https://freesound.org/people/zerokingfull/sounds/347410/',
        'title': 'Monster Roar 02', 'author': 'zerokingfull',
        'license': 'CC0-1.0', 'license_url': 'https://creativecommons.org/publicdomain/zero/1.0/',
        'verified': '2026-11-15'},
        'soil_source': {'author': 'Fantozzi', 'license': 'CC0-1.0',
            'url': 'https://opengameart.org/content/fantozzis-footsteps-grasssand-stone',
            'original_pack': 'https://www.freesound.org/people/Fantozzi/packs/10338/',
            'selection': 'SandL1, SandR1, SandL2, SandR2 only; stone clips are not used'},
        'processing': 'Soft sand contacts from CC0 Fantozzi recordings. Calls use only the CC0 zerokingfull capture WAV: separate pitch layers, vocal EQ, gentle compression and edge fades.',
        'inputs': [{'file': str(p.relative_to(ROOT)).replace('\\', '/'),
                    'sha256': hashlib.sha256(p.read_bytes()).hexdigest()}
                   for p in [ROOT/'assets/audio/jumpscare/monster-roar-02.ogg', ROOT/'assets/audio/jumpscare/scream.wav',
                             *sorted((BANK/'source/soil').glob('Fantozzi-*'))]],
        'files': files, 'preview_timeline': {'walk': [.35, 3.4], 'run': [4, 5.67], 'notice': [6.3, 8.14], 'roar': [9.5, 11.065]}}
    (BANK/'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({'files': len(files), 'min_rms': min(f['rms'] for f in files), 'max_peak': max(f['peak'] for f in files)}))


if __name__ == '__main__':
    main()
