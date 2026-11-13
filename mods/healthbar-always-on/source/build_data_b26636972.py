from pathlib import Path
import sys,hashlib,json,re,zipfile
import argparse
HERE=Path(__file__).resolve().parent
cli=argparse.ArgumentParser(description='Rebase three Healthbar v5 styles onto supplied build 26636972 inputs.')
for name in ('current','previous-downloads','output'):cli.add_argument('--'+name,type=Path,required=True)
args=cli.parse_args();C=args.current.resolve();O=args.output.resolve();A=O;S=HERE.parents[2]
O.mkdir(parents=True,exist_ok=True)
expected=json.loads((HERE/'healthbar-b26636972-inputs.json').read_text())
for path,digest in expected['inputs'].items():assert hashlib.sha256((C/path).read_bytes()).hexdigest()==digest,('Unsupported input',path)
for name,digest in expected['archives'].items():assert hashlib.sha256((args.previous_downloads/name).read_bytes()).hexdigest()==digest

sys.path.insert(0,str(S/'tooling'))
from crimson_mod_tools.build_204 import healthbar,BINARY
from crimson_mod_tools.staticinfo import Table
from crimson_mod_tools.dmm_builder import healthbar_ui
class Current:
    def table(self,name,key_size=4):return Table(*[(C/BINARY/(name+'.staticinfo'+s)).read_bytes() for s in ('body','header')],key_size)
files,patches,validation=healthbar(Current())
assert validation['preserved_skill_records']==2159
original=Current().table('skill');updated=Table(files[str(BINARY/'skill.staticinfobody').replace('\\','/')],files[str(BINARY/'skill.staticinfoheader').replace('\\','/')])
assert original.by_key.keys()==updated.by_key.keys()
assert [r.key for r in original.records if r.data!=updated.by_key[r.key].data]==[1201]
assert all(bytes.fromhex(p['original'])==Current().table('characterinfo').body[p['offset']:p['offset']+4] for p in patches)
result=[]
for variant,fid in [('multi',19042),('classic',19043),('caites',19044)]:
    html,css=healthbar_ui(C,variant)
    with zipfile.ZipFile(args.previous_downloads/f'2406_{fid}.zip') as z:
        payload={n:z.read(n) for n in z.namelist() if not n.endswith('.asi') and n!='BUILD_VALIDATION.txt'}
    key=next(n for n in payload if n.endswith('.json'));manifest=json.loads(payload.pop(key))
    manifest.update(version='5',patches=patches)
    manifest['description']=manifest['description'].split('Updated for')[0]+'Updated for Steam build 26636972; use exactly one style.'
    payload['Healthbar.json']=(json.dumps(manifest,indent=2)+'\n').encode()
    payload.update({'files/'+p:b for p,b in files.items()})
    payload['files/0012/ui/xml/gamemain/play/subtitletagview.html']=html
    payload['files/0012/ui/xml/gamemain/play/subtitletagview.css']=css
    folder=O/variant;folder.mkdir(parents=True,exist_ok=True)
    for p,data in payload.items():
        dest=folder/p;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(data)
    result.append(dict(variant=variant,html_sha256=hashlib.sha256(html).hexdigest(),css_sha256=hashlib.sha256(css).hexdigest(),native_required=variant!='classic'))
assert result[0]['html_sha256']==result[2]['html_sha256'] and len({x['css_sha256'] for x in result})==3
validation.update(build='26636972',variants=result,gameplay_tested=False)
(A/'healthbar-data-validation.json').write_text(json.dumps(validation,indent=2)+'\n')
print(json.dumps(validation,indent=2))
