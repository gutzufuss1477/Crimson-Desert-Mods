"""Artifact regression test. Usage: python test_tab_input.py path/to/v17.zip"""
import hashlib
import re
import sys
import zipfile
from fix_tab_input import CONTROLS, fix_compiled_ui, fix_html, string, strings

with zipfile.ZipFile(sys.argv[1]) as z:
    original = z.read('files/0012/ui/bin__/npcstorebuyview2_game.uibinary')
    html = z.read('files/0012/ui/xml/gamemain/play/npcstorebuyview2.html')
fixed = fix_compiled_ui(original)
fixed_html = fix_html(html)
for name, css, key in CONTROLS:
    head = strings(sorted(['cpp-able-layer', 'cpp-keyguide', 'inherit-pickable', 'keyguide', css]))
    tail = b''.join(string(s) for s in [name, '', 'KeyGuide.KeyGuideButton',
                    'KeyGuideButton', 'UIGamePlayControl_Common_KeyGuideButton', '']) + b'\x04\x00\x06'
    start = fixed.index(head)
    end = fixed.index(tail, start) + len(tail)
    control = fixed[start:end]
    assert control.count(string('keyMapName') + string(key)) == 1
    assert control.count(string('need-trigger-downonce-release') + string('True')) == 1
    assert string('need-trigger-downonce-release') + string('False') not in control
    tag = re.search(r'<widget\s+id="' + name + r'"[^>]*>', fixed_html.decode())[0]
    assert 'need-trigger-downonce-release="True"' in tag
assert len(fixed) == len(original)-2
# Foreign game builds, modified overlays, and accidental double application fail closed.
for fn, data in ((fix_html, fixed_html), (fix_compiled_ui, fixed),
                 (fix_html, html+b' '), (fix_compiled_ui, original+b'\0')):
    try:
        fn(data)
    except ValueError:
        pass
    else:
        raise AssertionError('Unexpected input accepted')
print('PASS: both compiled category controls and HTML use the native press/release gate; unrelated bytes preserved; wrong/double-patched inputs refused.')
print('Compiled UI SHA256:', hashlib.sha256(fixed).hexdigest())
