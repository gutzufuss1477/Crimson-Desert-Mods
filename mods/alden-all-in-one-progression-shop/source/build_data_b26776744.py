"""Rebuild one DMM ZIP from the previous Nexus release and locally extracted game data."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import zipfile

import lz4.block


def main():
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument('--variant', choices=['regular', 'dlc'], required=True)
    for name in ('current', 'previous', 'parser', 'asi', 'output'):
        cli.add_argument('--'+name, type=Path, required=True)
    args = cli.parse_args()
    profile = Path(__file__).parent/'native/profiles/b26776744'/args.variant
    expected = json.loads((profile/'package.json').read_text())
    sha = lambda b: hashlib.sha256(b).hexdigest()
    if sha(args.previous.read_bytes()) != expected['previous_sha256']:
        raise ValueError('Supply the previous Nexus file '+str(expected['previous_file_id']))
    spec = importlib.util.spec_from_file_location('dmm_parser', args.parser)
    parser = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(parser)
    with zipfile.ZipFile(args.previous) as archive:
        files = {p: archive.read(p) for p in archive.namelist() if not p.endswith('/')}
    del files['Preise.json']
    prefix = '0008/gamedata/binarystaticinfo__/bin/localstringinfo.staticinfo'
    def table(parts):
        return parser.parse_table('localstringinfo', *parts)
    game_keys = {r['key'] for r in table([(args.current/(prefix+p)).read_bytes() for p in ('body','header')])}
    mod_keys = {str(r['game_string']['index']) for r in table([files['files/'+prefix+p] for p in ('body','header')]) if r['key'] not in game_keys}
    for name in files:
        if name.endswith('.paloc'):
            source = (args.current/name.removeprefix('files/')).read_bytes()
            language = parser.parse_paloc_bytes(source)
            additions = [r for r in parser.parse_paloc_bytes(files[name]) if r['string_key'] in mod_keys]
            assert len(additions) == len(mod_keys) and not mod_keys.intersection(r['string_key'] for r in language)
            raw = parser.serialize_paloc(language + additions)
            block = lz4.block.compress(raw, store_size=False)
            header = bytearray(source[:512])
            struct.pack_into('<II', header, 9, len(block), len(raw))
            files[name] = bytes(header) + block
    files['Alden Category Tabs.asi'] = args.asi.read_bytes()
    files['manifest.json'] = (profile/'package-manifest.txt').read_bytes().replace(b'\r\n', b'\n')
    files['README.txt'] = (profile/'package-readme.txt').read_bytes().replace(b'\r\n', b'\n')
    if {p:sha(b) for p,b in files.items()} != expected['members']:
        raise ValueError('Rebuilt content differs from the released package')
    args.output.parent.mkdir(parents=True,exist_ok=True)
    tmp = args.output.with_suffix('.tmp')
    with zipfile.ZipFile(tmp,'w') as archive:
        for name,data in sorted(files.items()):
            archive.writestr(zipfile.ZipInfo(name,(2026,11,19,0,0,0)),data,compress_type=zipfile.ZIP_DEFLATED)
    with zipfile.ZipFile(tmp) as archive:
        assert archive.testzip() is None
    tmp.replace(args.output)


if __name__ == '__main__':
    main()
