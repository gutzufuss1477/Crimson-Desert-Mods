"""Validate the actual inserted subtree, including opaque unchanged events."""
from pathlib import Path
import re,sys
import ui_schema_b26636972 as schema
from ui_schema_b26636972 import Reader,strings
root=Path(__file__).resolve().parent
fixtures=root/(sys.argv[1] if len(sys.argv)>1 else 'fixtures')
prefix=strings(['!cpp-none','alden-category-bar','inherit-pickable'])
header=(root/'runtime_generated.hpp').read_text()
encoded=re.search(r'unsigned char After1\[\] = \{(.*?)\};',header,re.S).group(1)
payload=bytes(int(x,16) for x in re.findall(r'0x([a-f0-9]{2})',encoded))
assert payload[0]==1
vanilla=(fixtures/'vanilla.bin').read_bytes()
schema.configure_events(vanilla)
assert all(block in vanilla for block in schema.EVENT_BLOCKS)
for name in ['vanilla_expected','better_expected']:
    data=(fixtures/(name+'.bin')).read_bytes()
    assert data.count(prefix)==1
    reader=Reader(data,2);reader.pos=data.index(prefix);bar=reader.node()
    assert payload==b'\1'+bar.encode()
    properties=[n.optional[3] for n in bar.walk() if n.optional[3] is not None]
    assert properties==[('',0,1),('',0,1)]
legacy=(fixtures/'legacy_category_bar.bin').read_bytes()
try:Reader(legacy,2).node()
except (AssertionError,ValueError,UnicodeDecodeError):pass
else:raise AssertionError('Old malformed category subtree must be rejected')
print('PASS: complete inserted category tree uses current property schema; event blocks match fresh native UI; embedded payload matches fixtures; old subtree rejected.')
