"""Read-only, hash-guarded inputs for the runtime category builder.

Original game and third-party assets are supplied locally and are not included
in the source repository. Better Inventory is a verification input only.
"""
import hashlib
import zipfile
from pathlib import Path

from ui_binary import Reader, string

ALDEN_SHA256 = 'f4b7b6205074e11f92b621e99f4da959f61ff2b2098328016d99009e6925bd3b'
BETTER_SHA256 = '2801f4c45b4719b91d0cc641079e82db389d954c5d83eec279060d60a3cc1e5a'
HTML = 'files/0012/ui/xml/gamemain/play/npcstorebuyview2.html'
CSS = 'files/0012/ui/xml/gamemain/play/npcstore2.css'
BINARY = 'files/0012/ui/bin__/npcstorebuyview2_game.uibinary'
EXPECTED_OUTPUT_HASHES = [
    '6632acca3e3253b8bc61158abeff1ac6701589d668857b3d60144d05d5f64590',
    'adf7a251cec70e0f18984b91a8e395e8da839e39393a74395b8730337290c941',
]


def unique(data, needle):
    if data.count(needle) != 1:
        raise ValueError('Expected a unique compiled UI marker')
    return data.index(needle)


def read_archive(path, expected):
    if hashlib.sha256(path.read_bytes()).hexdigest() != expected:
        raise ValueError(f'Unsupported archive version: {path.name}')
    with zipfile.ZipFile(path) as z:
        if z.testzip() is not None:
            raise ValueError('Invalid archive CRC')
        result = {}
        for name in z.namelist():
            if name.endswith('/'):
                continue
            p = Path(name)
            if p.is_absolute() or '..' in p.parts or '\\' in name or ':' in name:
                raise ValueError('Unsafe archive path')
            if name in result:
                raise ValueError('Duplicate archive entry')
            result[name] = z.read(name)
        return result


def stylesheet_list(binary):
    # Build 26361232 stores stylesheet dependencies after the root tree.
    first = string('UI/GameMain/template/Currency.css')
    if first not in binary:
        raise ValueError('Missing native stylesheet dependency list')
    start = binary.index(first) - 4
    reader = Reader(binary)
    reader.pos = start
    paths = reader.strings()
    if (len(paths) != 46 or paths[-1] != 'UI/GameMain/Play/NpcStore2.css'
            or not all(p.endswith('.css') for p in paths)):
        raise ValueError('Unsupported stylesheet dependency list')
    return start, reader.pos, paths


def crlf(text):
    return text.replace('\r\n', '\n').replace('\r', '\n').replace('\n', '\r\n').encode('utf-8')
