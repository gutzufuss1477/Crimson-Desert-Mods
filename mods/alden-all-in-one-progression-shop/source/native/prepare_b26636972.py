"""Generate local-only Alden build inputs; never redistributes supplied game files.

Requires the current EXE/UI, the previous locally generated augmented UI for
the selected variant (17.3.4 regular / 17.4.3 DLC), legacy BI UI for a negative
test, and MinHook source. Outputs a self-contained build directory.
"""
from pathlib import Path
import argparse,hashlib,json,re,shutil,sys
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE.parent))
import build_runtime_shop as old
import ui_schema_b26636972 as schema

def sha(data):return hashlib.sha256(data).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--variant',choices=['regular','dlc'],required=True)
    for name in ['exe','ui','legacy-augmented','legacy-better','minhook','output']:p.add_argument('--'+name,type=Path,required=True)
    args=p.parse_args();profile=HERE/'profiles/b26636972'/args.variant
    manifest=json.loads((profile/'inputs.json').read_text())
    exe=args.exe.read_bytes();vanilla=args.ui.read_bytes();legacy=args.legacy_augmented.read_bytes()
    if sha(exe)!=manifest['exe_sha256']:raise ValueError('Unsupported EXE')
    if sha(vanilla)!=manifest['fixtures']['vanilla.bin']:raise ValueError('Unsupported vanilla UI')
    schema.configure_events(vanilla)
    prefix=schema.strings(['!cpp-none','alden-category-bar','inherit-pickable'])
    start=old.unique(legacy,prefix);reader=schema.Reader(legacy,1);reader.pos=start;bar=reader.node()
    oldbar=bar.encode()
    assert sha(oldbar)==manifest['fixtures']['legacy_category_bar.bin']
    properties=[n for n in bar.walk() if n.optional[3] is not None]
    assert len(properties)==2 and all(n.optional[3]==('',0) for n in properties)
    for node in properties:node.optional[3]=('',0,1)
    labels=[n for n in bar.walk() if n.optional[2] is not None]
    total=606 if args.variant=='regular' else 840
    assert len(labels)==(9 if args.variant=='regular' else 10)
    assert labels[0].optional[2]==[str(total+3)] and labels[5].optional[2]==['125']
    labels[0].optional[2]=[str(total)];labels[5].optional[2]=['122']
    patched_legacy=legacy[:start]+bar.encode()+legacy[reader.pos:]
    edits=old.make_edits(patched_legacy,vanilla)
    grid=schema.string('column')+schema.string('2')+schema.string('row')+schema.string('4')
    pos=old.unique(vanilla,grid)
    better=vanilla[:pos]+vanilla[pos:].replace(grid,schema.string('column')+schema.string('3')+schema.string('row')+schema.string('5'),1)
    fixtures={'vanilla.bin':vanilla,'better.bin':better,'vanilla_expected.bin':old.apply_edits(vanilla,edits),'better_expected.bin':old.apply_edits(better,edits),'legacy_better.bin':args.legacy_better.read_bytes(),'legacy_category_bar.bin':oldbar}
    for name,data in fixtures.items():
        if sha(data)!=manifest['fixtures'][name]:raise ValueError('Fixture mismatch: '+name)
    output=args.output.resolve()
    if output.exists() and any(output.iterdir()):raise ValueError('Use a new or empty output directory')
    output.mkdir(parents=True,exist_ok=True)
    for source in HERE.iterdir():
        if source.is_file() and source.suffix in ('.cpp','.hpp','.h','.rc','.cmd','.py') and source.name not in ('runtime_generated.hpp','consumer_test_generated.hpp','property_condition_generated.hpp'):
            shutil.copy2(source,output/source.name)
    for source in profile.iterdir():
        if source.suffix in ('.cpp','.hpp','.h'):shutil.copy2(source,output/source.name)
    arrays={'CssSource':(profile/'aldencategorytabs.css').read_bytes(),'After1':b'\1'+bar.encode()}
    regions={'NativeMemoryReadCode':0x1643cd0,'NativeStyleConsumerCode':0x392b740,'NativePropertyConditionCode':0x426cb10}
    for name,rva in regions.items():arrays[name]=old.exe_at(exe,rva,manifest['arrays'][name]['size'])
    for name,data in arrays.items():
        if sha(data)!=manifest['arrays'][name]['sha256']:raise ValueError('Generated array mismatch: '+name)
    for template in profile.glob('*.in'):
        text=template.read_text()
        for name,data in arrays.items():text=text.replace('@@'+name+'@@',old.array(name,data))
        assert '@@' not in text
        (output/template.stem).write_text(text)
    (output/'fixtures').mkdir()
    for name,data in fixtures.items():(output/'fixtures'/name).write_bytes(data)
    assert (args.minhook/'LICENSE.txt').is_file() and (args.minhook/'src/hook.c').is_file()
    shutil.copytree(args.minhook,output/'vendor/minhook',ignore=shutil.ignore_patterns('.git','build','out'))
    print('Prepared',args.variant,'with verified local headers and fixtures. Run build_runtime.cmd in',output)
if __name__=='__main__':main()
