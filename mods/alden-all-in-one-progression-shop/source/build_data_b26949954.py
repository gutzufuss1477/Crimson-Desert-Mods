"""Build the Alden data ZIP using clean game data and the released category helper."""
from pathlib import Path
import argparse, copy, hashlib, importlib.util, json, struct, sys, zipfile
import lz4.block
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE.parents[2]/'tooling'))
from crimson_mod_tools.staticinfo import Table
BIN='0008/gamedata/binarystaticinfo__/bin/'
def main(args):
    profile=HERE/'native/profiles/b26949954'/args.variant
    recipe=json.loads((profile/'package.json').read_text())
    sha=lambda b:hashlib.sha256(b).hexdigest()
    assert sha(args.previous.read_bytes())==recipe['previousSha256']
    spec=importlib.util.spec_from_file_location('dmm_parser',args.parser)
    parser=importlib.util.module_from_spec(spec);spec.loader.exec_module(parser)
    with zipfile.ZipFile(args.previous) as z:files={n:z.read(n) for n in z.namelist()}
    def pair(name,base):
        return [(base/(BIN+name+'.staticinfo'+s)).read_bytes() for s in ('header','body')]
    def save(name,h,b,side):
        hh,bb=parser.named_from_legacy(h,b,side)
        assert parser.named_to_legacy(hh,bb)[:2]==(h,b)
        for s,data in [('header',hh),('body',bb)]:files['files/'+BIN+name+'.staticinfo'+s]=data
    h,b,side=parser.named_to_legacy(*pair('storeinfo',args.current))
    record=Table(b,h,2).by_key[3101]
    merchant=parser.parse_table('storeinfo',record.data,struct.pack('<HHI',1,3101,0))[0]
    original=iter(merchant['stock_data_list']);offers=[]
    for template,key,saved in recipe['offers']:
        if template is None:
            row=copy.deepcopy(next(original));assert row['value']['payload']['body']==key
        else:
            row=copy.deepcopy(recipe['templates'][template])
            row['value']['payload']['body']=row['value']['raw_q']=key
        row['raw_d']=row['raw_e']=saved;offers.append(row)
    assert next(original,None) is None
    fields=[('buyable_stock_count',len(offers)),('stock_data_list',offers)]
    changes=[dict(key=3101,entry=merchant['string_key'],op='set',field=key,new=value) for key,value in fields]
    output=parser.apply_intents('storeinfo',b,h,changes)
    assert all(r['status']=='applied' for r in output['outcomes'])
    save('storeinfo',output['pabgh'] or h,output['body'],side)
    name='localstringinfo'
    prior=parser.parse_table(name,*[files['files/'+BIN+name+'.staticinfo'+s] for s in ('body','header')])
    additions=[r for r in prior if r['key'] in recipe['labelKeys']]
    h,b,side=parser.named_to_legacy(*pair(name,args.current))
    now=parser.parse_table(name,b,h)
    assert len(additions)==len(recipe['labelKeys']) and not set(recipe['labelKeys']).intersection(r['key'] for r in now)
    header=bytearray(struct.pack('<H',len(now)+len(additions))+h[2:])
    for row in additions:
        header.extend(struct.pack('<II',row['key'],len(b)))
        b+=parser.serialize_table(name,[row])
    save(name,bytes(header),b,side)
    labels={str(r['game_string']['index']) for r in additions}
    for n,data in list(files.items()):
        if not n.endswith('.paloc'):continue
        raw=(args.current/n.removeprefix('files/')).read_bytes()
        base=parser.parse_paloc_bytes(raw)
        extra=[r for r in parser.parse_paloc_bytes(data) if r['string_key'] in labels]
        assert len(extra)==len(labels) and not labels.intersection(r['string_key'] for r in base)
        payload=parser.serialize_paloc(base+extra);block=lz4.block.compress(payload,store_size=False)
        header=bytearray(raw[:512]);struct.pack_into('<II',header,9,len(block),len(payload))
        files[n]=bytes(header)+block
    files['Alden Category Tabs.asi']=args.asi.read_bytes()
    for name in ('manifest.json','README.txt'):
        files[name]=(profile/('package-'+name+'.txt')).read_bytes().replace(b'\r\n',b'\n')
    for name in files:
        if name!='Alden Category Tabs.asi' and sha(files[name])!=recipe['members'][name]:raise ValueError(name)
    assert set(files)==set(recipe['members'])
    args.output.parent.mkdir(parents=True,exist_ok=True);tmp=args.output.with_suffix('.tmp')
    with zipfile.ZipFile(tmp,'w') as z:
        for n,data in sorted(files.items()):z.writestr(zipfile.ZipInfo(n,(2026,11,27,0,0,0)),data,compress_type=zipfile.ZIP_DEFLATED)
    with zipfile.ZipFile(tmp) as z:assert z.testzip() is None
    tmp.replace(args.output)
if __name__=='__main__':
    cli=argparse.ArgumentParser(description=__doc__)
    cli.add_argument('--variant',choices=['regular','dlc'],required=True)
    for name in ['current','previous','parser','asi','output']:cli.add_argument('--'+name,type=Path,required=True)
    main(cli.parse_args())
