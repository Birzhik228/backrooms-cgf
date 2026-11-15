"""Check approved asset provenance and write/check the complete release inventory.

Uses the audited audio registry and original procedural model manifest. Unknown
binary assets fail closed; this script does not infer a license from a filename.
Python standard library only. Run after intentional asset or credit edits.
"""
from pathlib import Path
import argparse
import hashlib
import json

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / 'assets'
OUTPUT = ASSETS / 'asset-license-manifest.json'

def read_json(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def inventory():
    audio = read_json(ASSETS / 'audio/manifest.json')
    model = read_json(ASSETS / 'models/aranea/aranea-manifest.json')
    assert model['name'] == 'Threshold Sentinel' and model['license'] == 'CC0-1.0'
    assert digest(ROOT / model['generator']) == model['generatorSha256'], 'Model generator hash differs'
    approved = {}
    for item in audio['files']:
        relative = 'audio/' + item['file']
        assert relative not in approved, 'Duplicate audio record'
        assert item['license'] == 'CC0-1.0', 'Unapproved audio license'
        sources = [audio['sources'][key] for key in item['sources']]
        approved[relative] = dict(
            sha256=item['sha256'], bytes=item['bytes'], license=item['license'],
            source='; '.join(source.get('url', source.get('generator', '')) for source in sources),
            creator='; '.join(dict.fromkeys(source['author'] for source in sources)),
            kind=item['kind'])
    for filename, item in model['files'].items():
        approved['models/aranea/' + filename] = dict(
            sha256=item['sha256'], bytes=item['bytes'], license='CC0-1.0',
            source=model['generator'], creator='Backrooms CGF project with disclosed Codex assistance',
            kind='Original procedural mesh, animation or texture')
    previews = {'preview.png', 'preview-level-fun.png', 'preview-parking.png',
                'preview-city-ending.png', 'models/aranea/preview.png'}
    files = []
    found = set()
    for path in sorted(ASSETS.rglob('*')):
        if path.is_symlink():
            raise ValueError('Linked asset is not accepted: ' + str(path))
        if not path.is_file() or path == OUTPUT:
            continue
        relative = path.relative_to(ASSETS).as_posix()
        actual = dict(path=relative, bytes=path.stat().st_size, sha256=digest(path))
        if relative in approved:
            provenance = approved[relative]
            assert actual['bytes'] == provenance['bytes'], 'Asset size changed: ' + relative
            assert actual['sha256'].lower() == provenance['sha256'].lower(), 'Asset hash changed: ' + relative
            actual.update({key: value for key, value in provenance.items() if key not in ('bytes', 'sha256')})
            found.add(relative)
        elif relative in previews:
            actual.update(license='Project screenshot included with this submission',
                source='Project renderer; current generated environment and creature assets',
                creator='Backrooms CGF project with disclosed Codex assistance', kind='Project screenshot')
        elif path.suffix.lower() in ('.md', '.txt', '.json'):
            actual.update(license='Project attribution and documentation included with this submission',
                source='Project metadata; THIRD_PARTY.md and the associated asset provenance',
                creator='Backrooms CGF project with disclosed Codex assistance', kind='Attribution or generation metadata')
        else:
            raise ValueError('Binary asset has no audited source entry: ' + relative)
        assert actual['source'] and actual['creator'], 'Incomplete attribution: ' + relative
        files.append(actual)
    assert found == set(approved), 'Approved payload is missing: ' + repr(set(approved) - found)
    return dict(schema=1, auditedDate='2026-11-15',
        scope='Submission assets only; excludes Git history, backups and earlier captures/builds.',
        sources='THIRD_PARTY.md; docs/asset-permissions-2026-11-15.md; assets/audio/manifest.json; assets/models/aranea/aranea-manifest.json',
        files=files)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--write', action='store_true', help='Write inventory after validating all payload sources')
    options = parser.parse_args()
    result = inventory()
    if options.write:
        OUTPUT.write_text(json.dumps(result, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    else:
        recorded = read_json(OUTPUT)
        assert recorded == result, 'Asset inventory is stale; review changes before regenerating it'
    print('PASS audited asset inventory:', len(result['files']), 'files')

if __name__ == '__main__':
    main()
