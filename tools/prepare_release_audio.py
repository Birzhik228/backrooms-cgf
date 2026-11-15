"""Rebuild release audio from documented CC0 inputs and original synthesis.

Offline dependency: NumPy. No downloads, commercial recordings or reference
imitations. Newly synthesized music/pickup audio is dedicated to CC0.
"""
from pathlib import Path
import argparse
import hashlib
import json
import wave
import numpy as np
import prepare_aranea_audio as creature

ROOT = Path(__file__).resolve().parents[1]
AUDIO = ROOT / 'assets/audio'
RATE = 22050
CC0 = 'https://creativecommons.org/publicdomain/zero/1.0/'
SOURCES = {
    'gbox': {'author': 'GboxMikeFozzy', 'title': 'Footsteps',
             'url': 'https://opengameart.org/content/footsteps-0'},
    'sand': {'author': 'Fantozzi', 'title': 'Grass, Sand, Stone footsteps',
             'url': 'https://opengameart.org/content/fantozzis-footsteps-grasssand-stone'},
    'roar': {'author': 'zerokingfull', 'title': 'Monster Roar 02',
             'url': 'https://freesound.org/people/zerokingfull/sounds/347410/'},
    'original': {'author': 'Backrooms CGF project with disclosed Codex assistance',
                 'title': 'Original procedural music and pickup chime',
                 'generator': 'tools/prepare_release_audio.py',
                 'inputs': 'Oscillators and seeded noise only; no recordings or copied melody.'}}

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def write(path, samples):
    path.parent.mkdir(parents=True, exist_ok=True)
    pcm = np.rint(np.clip(samples, -.9, .9) * 32767).astype('<i2')
    pcm[0] = pcm[-1] = 0
    channels = 1 if pcm.ndim == 1 else pcm.shape[1]
    with wave.open(str(path), 'wb') as out:
        out.setparams((channels, 2, RATE, 0, 'NONE', 'not compressed'))
        out.writeframes(pcm.tobytes())
    return {'file': path.relative_to(AUDIO).as_posix(), 'sha256': sha(path),
            'sample_rate': RATE, 'channels': channels, 'samples': len(pcm),
            'seconds': len(pcm)/RATE, 'peak': int(np.max(np.abs(pcm.astype(int)))),
            'rms': float(np.sqrt(np.mean(pcm.astype(float)**2)))}

def manifest(directory, source, files, processing):
    value = {'license': 'CC0-1.0', 'license_url': CC0,
             'source': SOURCES[source], 'verified': '2026-11-15',
             'processing': processing, 'files': files}
    (AUDIO/directory/'manifest.json').write_text(json.dumps(value, indent=2)+'\n', encoding='utf-8')

def prepare_footsteps():
    player = []
    for index in range(1, 7):
        name = f'step-{index:02}.wav'
        source, target = AUDIO/'footsteps'/name, AUDIO/'footsteps-player'/name
        target.parent.mkdir(parents=True, exist_ok=True)
        # These CC0 takes have sharp peaks and quiet shoe-contact bodies.
        # Gentle transient compression retains the recording while keeping
        # the existing Partygoer normalization and distant audibility stable.
        contact = np.tanh(creature.read(source)*4)
        contact *= .88/max(np.max(np.abs(contact)), 1e-9)
        record = write(target, contact)
        record.update({'input': 'assets/audio/footsteps/'+name, 'input_sha256': sha(source)})
        player.append(record)
    manifest('footsteps-player', 'gbox', player,
             'Existing CC0 processed shoe contacts, gently transient-compressed with tanh(x*4) and normalized to 0.88 peak; original OGGs and original conversion manifest remain in ../footsteps.')
    monster = [write(AUDIO/'footsteps-monster'/f'step-{i:02}.wav', clip)
               for i, clip in enumerate(creature.make_steps(True), 1)]
    manifest('footsteps-monster', 'sand', monster,
             'Fantozzi CC0 sand contacts, pitch/spectral filtering through prepare_aranea_audio.make_steps(True).')

def ambient_music():
    duration = 48
    t = np.arange(RATE*duration)/RATE
    rng = np.random.default_rng(150112026)
    voices = []
    # Original slow electrical drone: authored frequencies and envelopes,
    # with no transcription or imitation of a pre-existing soundtrack.
    for side in (0, 1):
        sample = np.zeros_like(t)
        for index, frequency in enumerate((55., 82.4069, 110., 164.8138, 220., 329.6276)):
            drift = .15*np.sin(2*np.pi*t/(19+index*2)+side*.7)
            phase = 2*np.pi*(frequency*t+np.cumsum(drift)/RATE)+index*.47+side*.19
            breath = .65+.35*np.sin(2*np.pi*t/(11+index*3)+index*.8)**2
            sample += .12/(1+index*.65)*np.sin(phase)*breath
        noise = rng.normal(0, 1, len(t))
        frequency = np.fft.rfftfreq(len(t), 1/RATE)
        spectrum = np.exp(-(frequency/1100)**2)*(1-np.exp(-(frequency/90)**2))
        air = np.fft.irfft(np.fft.rfft(noise)*spectrum, n=len(t))
        sample += .045*air*(.5+.5*np.sin(2*np.pi*t/17+side*.4)**2)
        for start, frequency in ((7.5, 440.), (19., 392.), (33.5, 523.2511)):
            local = np.maximum(0, t-start-side*.07)
            envelope = np.where(t>start+side*.07, (1-np.exp(-local/1.2))*np.exp(-local/3.8), 0)
            sample += .038*envelope*(np.sin(2*np.pi*frequency*local)+.3*np.sin(2*np.pi*frequency*2.007*local))
        voices.append(sample)
    samples = np.column_stack(voices)
    samples -= samples.mean(axis=0)
    samples *= .24/max(np.max(np.abs(samples)), 1e-9)
    fade = np.sin(np.minimum(t/2.5, 1)*np.pi/2)**2
    fade *= np.sin(np.minimum((duration-1/RATE-t)/2.5, 1)*np.pi/2)**2
    return samples*fade[:, None]

def pickup_chime():
    t = np.arange(RATE*2)/RATE
    result = np.zeros_like(t)
    for start, frequency, level in ((0., 659.2551, .55), (.18, 830.6094, .46), (.40, 1108.7305, .36)):
        local = np.maximum(0, t-start)
        envelope = np.where(t>=start, (1-np.exp(-local/.005))*np.exp(-local/1.0), 0)
        result += level*envelope*(np.sin(2*np.pi*frequency*local)+.18*np.sin(2*np.pi*frequency*2.01*local))
    return creature.normalize(creature.edges(result, .003, .20), .18, .68)

def party_capture():
    source = creature.read(AUDIO/'jumpscare/scream.wav')
    bright = creature.band(creature.pitched(source, 1.17, 2*RATE), 170, 6200)
    low = creature.band(creature.pitched(source, .69, 2*RATE), 70, 1400)
    return creature.normalize(creature.edges(.78*bright+.25*low, .006, .14), .25, .80)

def registry():
    entries = []
    # A directory name is not a license: accept only the reviewed payload list.
    allowed = {f'footsteps/source/{name}' for name in
               ['01-footstep_0.ogg']+[f'{i:02}-footstep.ogg' for i in range(2, 7)]}
    allowed |= {f'{bank}/step-{i:02}.wav' for bank, count in
                [('footsteps', 6), ('footsteps-player', 6), ('footsteps-monster', 4)]
                for i in range(1, count+1)}
    allowed |= {f'aranea/source/soil/Fantozzi-{name}.{extension}'
                for name in creature.SOIL_NAMES for extension in ('wav', 'flac')}
    allowed |= {f'aranea/{kind}-{i:02}.wav' for kind in ('walk', 'run', 'notice', 'roar') for i in range(1, 5)}
    allowed |= {'aranea/preview.wav', 'jumpscare/monster-roar-02.ogg', 'jumpscare/scream.wav',
                'music/threshold-ambient.wav', 'partygoer/jumpscare.wav', 'verity/pickup.wav'}
    for path in sorted(AUDIO.rglob('*')):
        if path.suffix.lower() not in {'.wav', '.ogg', '.flac', '.mp3', '.m4a'}:
            continue
        relative = path.relative_to(AUDIO).as_posix()
        if relative not in allowed:
            raise RuntimeError('Unreviewed audio payload: '+relative)
        if relative.startswith(('music/threshold-', 'verity/')):
            sources = ['original']
        elif relative.startswith(('footsteps/', 'footsteps-player/')):
            sources = ['gbox']
        elif relative.startswith(('footsteps-monster/', 'aranea/walk-', 'aranea/run-', 'aranea/source/soil/')):
            sources = ['sand']
        elif relative.startswith(('jumpscare/', 'partygoer/', 'aranea/notice-', 'aranea/roar-')):
            sources = ['roar']
        elif relative == 'aranea/preview.wav':
            sources = ['sand', 'roar']
        else:
            raise RuntimeError('Unexpected audio asset without explicit provenance: '+relative)
        kind = 'original synthesis' if sources == ['original'] else 'original recording' if path.suffix in {'.ogg', '.flac'} else 'derived recording'
        entries.append({'file': relative, 'bytes': path.stat().st_size, 'sha256': sha(path),
                        'license': 'CC0-1.0', 'license_url': CC0, 'kind': kind, 'sources': sources})
    if {entry['file'] for entry in entries} != allowed:
        raise RuntimeError('The reviewed audio bank is incomplete.')
    result = {'schema': 1, 'verified': '2026-11-15', 'license_url': CC0,
              'sources': SOURCES, 'files': entries,
              'note': 'CC0 only. Original synthesis dedication covers new music and chime; recordings retain named source authors.'}
    (AUDIO/'manifest.json').write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
    return result

def main():
    argparse.ArgumentParser(description=__doc__).parse_args()
    prepare_footsteps()
    creature.main()
    music = write(AUDIO/'music/threshold-ambient.wav', ambient_music())
    manifest('music', 'original', [music], 'Original stereo oscillator/noise composition, 48 seconds, 2.5-second edge fades.')
    pickup = write(AUDIO/'verity/pickup.wav', pickup_chime())
    manifest('verity', 'original', [pickup], 'Original two-second synthesized three-tone pickup chime, no voice or song sample.')
    party = write(AUDIO/'partygoer/jumpscare.wav', party_capture())
    manifest('partygoer', 'roar', [party], 'Two filtered/pitched layers of the CC0 scream with bounded gain and fades.')
    result = registry()
    print(json.dumps({'audio_assets': len(result['files']), 'all_licenses': 'CC0-1.0', 'music': music, 'pickup': pickup}))

if __name__ == '__main__':
    main()
