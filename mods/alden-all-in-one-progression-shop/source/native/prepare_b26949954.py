"""Prepare Alden's native source and UI fixtures from local retail inputs."""
from pathlib import Path
import argparse, hashlib, json, re, shutil, struct, sys

HERE = Path(__file__).resolve().parent
def sha(data):
    return hashlib.sha256(data).hexdigest()

def prepare(args):
    profile = HERE / 'profiles/b26949954' / args.variant
    recipe = json.loads((profile/'ui-recipe.json').read_text())
    out = args.output
    out.mkdir(parents=True, exist_ok=True)
    for name in recipe['sources']:
        path = profile/name
        shutil.copy2(path if path.is_file() else HERE/name, out/name)
    sys.path.insert(0, str(out))
    import ui_schema_b26636972 as previous
    from ui_schema_26949954 import encode
    vanilla = args.ui.read_bytes()
    previous.configure_events(vanilla)
    fixtures = {'vanilla.bin':vanilla,
                'previous_vanilla.bin':(args.previous_fixtures/'vanilla.bin').read_bytes(),
                'previous_alden.bin':(args.previous_fixtures/'vanilla_expected.bin').read_bytes()}
    for name, data in fixtures.items():
        if sha(data) != recipe['fixtures'][name]:
            raise ValueError('Unexpected input for '+name)
    src = fixtures['previous_alden.bin']
    reader = previous.Reader(src)
    reader.pos = src.index(previous.strings(['!cpp-none','alden-category-bar','inherit-pickable']))
    bar = reader.node()
    labels = [n.optional[2] for n in bar.walk() if n.optional[2] is not None]
    for index in (0,5):
        labels[index][0] = str(int(labels[index][0])-1)
    s = previous.string
    grid = s('column')+s('2')+s('row')+s('4')
    assert vanilla.count(grid)==1
    fixtures['better.bin'] = vanilla.replace(grid,s('column')+s('3')+s('row')+s('5'))
    # Preserve CRLF from the authored stylesheet when embedded.
    css = (HERE/'profiles/b26776744'/args.variant/'aldencategorytabs.css').read_bytes().decode('utf-8-sig').encode()
    values = {'BuySourceTail':vanilla[-recipe['tailSize']:]}
    for name in ('vanilla','better'):
        buf = fixtures[name+'.bin']
        changes = []
        for index,(offset,length) in enumerate(recipe['edits'][name]):
            before = buf[offset:offset+length]
            if index == 0: after = previous.number(6)
            elif index == 1: after = b'\1'+encode(bar)
            elif index == 2: after = previous.number(40)
            elif index == 3: after = s('selector-itemlist-tabbar')+s('#AldenCategoryTabs')
            else:
                expanded = before[4:]+b'\r\n'+css
                after = previous.number(len(expanded))+expanded
            for side,data in [('from',before),('to',after)]:
                if data: values[f'{name}_{side}_{index}'] = data
            changes.append((offset,length,after))
        for offset,length,after in sorted(changes,reverse=True):
            buf=buf[:offset]+after+buf[offset+length:]
        fixtures[name+'_expected.bin']=buf
    text=(profile/'runtime_generated.hpp.in').read_text()
    for name,data in values.items():
        assert sha(data)==recipe['arrays'][name],name
        text=text.replace('@@'+name+'@@',','.join(f'0x{b:02x}' for b in data))
    assert '@@' not in text
    (out/'runtime_generated.hpp').write_text(text)
    (out/'fixtures').mkdir(exist_ok=True)
    for name,data in fixtures.items():
        assert sha(data)==recipe['fixtures'][name],name
        (out/'fixtures'/name).write_bytes(data)
    assert (args.minhook/'src/hook.c').is_file() and (args.minhook/'LICENSE.txt').is_file()
    shutil.copytree(args.minhook,out/'vendor/minhook',dirs_exist_ok=True,ignore=shutil.ignore_patterns('.git','out','build'))

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--variant',choices=('regular','dlc'),required=True)
    for field in ('ui','previous-fixtures','minhook','output'):
        parser.add_argument('--'+field,type=Path,required=True)
    prepare(parser.parse_args())
