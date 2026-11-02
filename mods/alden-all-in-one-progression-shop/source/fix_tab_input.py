"""Correct the two v17 category keyguides in author-supplied UI files.

The compiled controls were copied from MoveItemAxis3 and retained its False
need-trigger-downonce-release attribute. Native TabbarWithKeyguide3 uses True
(the default). False dispatches both DownOnce and Release as tab activations.
No game assets are included; the complete v17 package is the input.
"""
import hashlib
import re
import struct

V17_HTML_SHA256 = 'aca740ed39003eba089b408e1c1f0d297c4aea16be5533a4f664682b4d7f9188'
V17_BINARY_SHA256 = '1cc3f443992213aa34b25b0cecebdeec3196299beeee7890d0ac3a1568a4e7e4'
CONTROLS = [('AldenTabPrevious', 'alden-tab-prev', 'SubMenuMoveLeft8'),
            ('AldenTabNext', 'alden-tab-next', 'SubMenuMoveRight8')]


def string(value):
    data = value.encode('utf-8')
    return struct.pack('<I', len(data)) + data


def strings(values):
    return struct.pack('<I', len(values)) + b''.join(string(s) for s in values)


def unique(data, needle):
    if data.count(needle) != 1:
        raise ValueError('Expected exactly one matching UI control/attribute')
    return data.index(needle)


def fix_html(data):
    if hashlib.sha256(data).hexdigest() != V17_HTML_SHA256:
        raise ValueError('Expected the released v17 shop HTML')
    text = data.decode('utf-8')
    for name, _, key in CONTROLS:
        pattern = re.compile(r'<widget\s+id="' + name + r'"[^>]*>')
        matches = pattern.findall(text)
        if len(matches) != 1 or f'keyMapName="{key}"' not in matches[0]:
            raise ValueError('Expected v17 category keyguide')
        before = matches[0]
        if 'need-trigger-downonce-release' in before:
            raise ValueError('Unexpected input override already present')
        after = before[:-1] + ' need-trigger-downonce-release="True">'
        text = text.replace(before, after, 1)
    return text.encode('utf-8')


def fix_compiled_ui(data):
    if hashlib.sha256(data).hexdigest() != V17_BINARY_SHA256:
        raise ValueError('Expected the released v17 compiled shop UI')
    before = string('need-trigger-downonce-release') + string('False')
    after = string('need-trigger-downonce-release') + string('True')
    edits = []
    for name, css, key in CONTROLS:
        prefix = strings(sorted(['cpp-able-layer', 'cpp-keyguide',
                                 'inherit-pickable', 'keyguide', css]))
        suffix = b''.join(string(s) for s in [name, '', 'KeyGuide.KeyGuideButton',
                         'KeyGuideButton', 'UIGamePlayControl_Common_KeyGuideButton', ''])
        suffix += b'\x04\x00\x06'
        start, end = unique(data, prefix), unique(data, suffix) + len(suffix)
        if end <= start:
            raise ValueError('Invalid compiled keyguide boundaries')
        control = data[start:end]
        unique(control, string('keyMapName') + string(key))
        edits.append(start + unique(control, before))
    result = data
    for at in sorted(edits, reverse=True):
        result = result[:at] + after + result[at+len(before):]
    # Recover the exact input, proving that only these two attribute values changed.
    restored = result
    delta = len(after) - len(before)
    for index, at in reversed(list(enumerate(sorted(edits)))):
        adjusted = at + index * delta
        if restored[adjusted:adjusted+len(after)] != after:
            raise ValueError('Compiled keyguide edit verification failed')
        restored = restored[:adjusted] + before + restored[adjusted+len(after):]
    if restored != data:
        raise ValueError('Unexpected compiled UI modification')
    return result
