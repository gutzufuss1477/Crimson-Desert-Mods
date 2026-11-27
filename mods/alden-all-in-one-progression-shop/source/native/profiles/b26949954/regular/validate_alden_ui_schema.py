"""Check Alden against byte-for-byte reference widgets from the shipping game."""
from pathlib import Path
import re,sys
import ui_schema_b26636972 as old
from ui_schema_26949954 import Reader,encode,strings
root=Path(__file__).resolve().parent
fixtures=root/(sys.argv[1] if len(sys.argv)>1 else 'fixtures')
vanilla=(fixtures/'vanilla.bin').read_bytes()
old.configure_events(vanilla)
retail=old.Reader((fixtures/'previous_vanilla.bin').read_bytes())
guide=strings(['cpp-keyguide-group','cpp-show-layer','keyguide-horizontal-section','stack-vertical'])
retail.pos=retail.data.index(guide)
reference=retail.node()
sample=encode(reference)
reader=Reader(vanilla)
reader.pos=vanilla.index(guide);start=reader.pos
current=reader.node()
assert vanilla[start:reader.pos]==sample and encode(current)==sample
assert current.id=='StoreKeyGuideWrap' and len(list(current.walk()))==136
assert sum(n.optional[1] is not None for n in current.walk())==33
assert sum(n.names[-1].startswith('!') for n in current.walk())==22

prefix=strings(['!cpp-none','alden-category-bar','inherit-pickable'])
following=strings(['two-column-list-section-wrap'])
previous=old.Reader((fixtures/'previous_alden.bin').read_bytes())
previous.pos=previous.data.index(prefix);baseline=previous.node()
previous_nodes=list(baseline.walk())
for name in ('vanilla','better'):
    header=(root/'runtime_generated.hpp').read_text()
    body=re.search('unsigned char '+name+r'_to_1\[\]=\{(.*?)\};',header,re.S)[1]
    compiled=bytes.fromhex(' '.join(re.findall(r'0x([0-9a-f]{2})',body)))
    buf=(fixtures/(name+'_expected.bin')).read_bytes()
    r=Reader(buf);r.pos=buf.index(prefix);start=r.pos
    bar=r.node();nodes=list(bar.walk())
    assert buf[start-1:r.pos]==compiled==bytes([1])+encode(bar)
    assert r.pos==buf.index(following)-1 and len(nodes)==len(previous_nodes)
    for a,b in zip(nodes,previous_nodes):
        assert (a.css,a.attrs,a.names,a.flags)==(b.css,b.attrs,b.names,b.flags)
        for i,(cur,prev) in enumerate(zip(a.optional,b.optional)):
            if i==2 and prev in (['840'],['606'],['122']):
                assert cur==[str(int(prev[0])-1)]
            else:assert cur==prev
    assert [n.optional[3] for n in nodes if n.optional[3] is not None]==[('',0,1)]*2
    # The previous build encoded every node using the earlier title and condition schema.
    try:Reader(baseline.encode()).node()
    except (ValueError,AssertionError,UnicodeError):pass
    else:raise AssertionError('Broken title encoding was accepted')
print('PASS current game widget (136 nodes, 33 events, 22 negated conditions), both grids, every authored category field, and rejection of the previous title format.')
