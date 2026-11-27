"""Build all three 2.06 HUD variants using locally extracted game assets."""
from pathlib import Path
import argparse, hashlib, importlib.util, json, re, struct, sys, tempfile, zipfile
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE.parents[2]/'tooling'))
from crimson_mod_tools.staticinfo import Table
from crimson_mod_tools.dmm_builder import healthbar_ui
BIN='0008/gamedata/binarystaticinfo__/bin/'
UI='0012/ui/xml/gamemain/play/'
def enc(x):return (json.dumps(x,indent=2,ensure_ascii=False)+'\n').encode()
def sha(b):return hashlib.sha256(b).hexdigest()
def archive(path,files):
    path.parent.mkdir(parents=True,exist_ok=True)
    temp=path.with_suffix('.tmp')
    with zipfile.ZipFile(temp,'w') as z:
        for n,b in sorted(files.items()):
            z.writestr(zipfile.ZipInfo(n,(2026,11,27,0,0,0)),b,compress_type=zipfile.ZIP_DEFLATED)
    with zipfile.ZipFile(temp) as z:assert z.testzip() is None and {n:z.read(n) for n in z.namelist()}==files
    temp.replace(path)
    print('Ready:',path,flush=True)
def table(name,kind=None):
    root=current
    b,h=[(root/(BIN+name+'.staticinfo'+s)).read_bytes() for s in ('body','header')]
    hh,bb,side=parser.named_to_legacy(h,b)
    assert parser.named_from_legacy(hh,bb,side)==(h,b)
    return hh,bb,side
def apply(name,h,b,intents):
    result=parser.apply_intents(name,b,h,intents)
    assert all(x['status']=='applied' for x in result['outcomes']),result['outcomes']
    return result['pabgh'] or h,result['body']
def v3(title,ver,desc,name,intents):
    return {'format':3,'format_minor':1,'modinfo':dict(title=title,author='Blablup',version=ver,description=desc),'targets':[dict(file=name+'.pabgb',intents=intents)]}
def stage():
    h,b,side=table('skill'); skills=Table(b,h)
    target=skills.require(1201,'Passive_Player_BasicSkill')
    donor=skills.require(91002,'Equip_Passive_ShowHPUI')
    donor_row=parser.parse_table('skill',donor.data,struct.pack('<HII',1,donor.key,0))[0]
    assert donor_row['skill_group_key']==donor.key
    payload=donor.data[donor.prefix_size:]
    token=struct.pack('<I',donor.key)
    assert payload.count(token)==1
    updated=skills.replace({target.key:target.data[:target.prefix_size]+payload.replace(token,struct.pack('<I',target.key))})
    nh,nb=parser.named_from_legacy(updated.header,updated.body,side)
    th,tb,_=parser.named_to_legacy(nh,nb)
    assert tb==updated.body and th==updated.header and len(skills.records)==2163
    modrow=parser.parse_table('skill',updated.by_key[target.key].data,struct.pack('<HII',1,target.key,0))[0]
    assert modrow==dict(donor_row,key=target.key,string_key=target.name,skill_group_key=target.key)
    h,b,cs=table('characterinfo'); chars=Table(b,h)
    intents=[]
    for key in (1,1002113,1004085):
        r=chars.by_key[key]; obj=parser.parse_table('characterinfo',r.data,struct.pack('<HII',1,key,0))[0]
        assert obj['of_origin_skills'][0]['skill']==250
        intents.append(dict(key=key,entry=r.name,op='set',field='of_origin_skills[0].skill',new=91002))
    mh,mb=apply('characterinfo',h,b,intents)
    pair=parser.named_from_legacy(mh,mb,cs)
    assert parser.named_to_legacy(*pair)[:2]==(mh,mb)
    changed=Table(mb,mh)
    assert {r.key for r in chars.records if r.data!=changed.by_key[r.key].data}=={1,1002113,1004085}
    # The current UI inlines the old CSS. Apply the existing style recipe to that
    # block, then put the result back where this build's UI loads it.
    html=(current/(UI+'subtitletagview.html')).read_text(encoding='utf-8-sig')
    m=re.search(r'(<style>)(.*?)(</style>)',html,re.S);assert m and html.count('<style>')==1
    source=A/'ui-input'/UI;source.mkdir(parents=True,exist_ok=True)
    (source/'subtitletagview.html').write_text(html[:m.start(2)]+'__INLINE_HEALTHBAR_STYLE__'+html[m.end(2):],encoding='utf8')
    (source/'subtitletagview.css').write_text(m[2],encoding='utf8')
    for variant,label in [('classic','Classic Single Target'),('multi','Vanilla Multitarget'),('caites','Caites Multitarget')]:
        out,styles=healthbar_ui(A/'ui-input',variant)
        out=out.replace(b'__INLINE_HEALTHBAR_STYLE__',styles.decode('utf-8-sig').encode())
        assert b'<style>' in out and b'ShowHPBuffTag="CheckShowHP"' in out
        desc='Always-on enemy health bars for game 2.06.00.'
        files={'Healthbar.json':enc(v3('Healthbar - '+label,'6',desc,'characterinfo',intents)),
               'files/'+BIN+'skill.staticinfobody':nb,'files/'+BIN+'skill.staticinfoheader':nh,
               'files/'+UI+'subtitletagview.html':out,
               'README.txt':('Healthbar '+label+' v6 | Steam 26949954 | DMM 5.3.2\nClose the game and unmount. Replace the previous Healthbar and use only one style. Import the complete ZIP, enable, mount and restart. '+('Disable any previous multitarget helper.' if variant=='classic' else 'Enable the included Healthbar Multitarget under ASI Mods.')+'\n').encode()}
        staging=A/'healthbar'/variant
        for n,data in files.items():
            path=staging/n;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
def num(n):return struct.pack('<I',n)
def string(s):
    if isinstance(s,str):s=s.encode()
    return num(len(s))+s
def strings(seq):return num(len(seq))+b''.join(map(string,seq))
def pos(b,needle):
    assert b.count(needle)==1,(needle[:65],b.count(needle))
    return b.index(needle)
def getstring(b,p):
    size=struct.unpack_from('<I',b,p)[0]
    if size>512 or size+p+4>len(b):raise ValueError()
    return b[p+4:p+4+size].decode('utf-8'),p+4+size
def attributes(b,pair):
    needle=string(pair[0])+string(pair[1]); hit=pos(b,needle);candidates=[]
    for p in range(max(1,hit-2048),hit+1):
        if b[p-1]!=1:continue
        count=struct.unpack_from('<I',b,p)[0]
        if not 1<=count<=100:continue
        attrs=[];q=p+4
        try:
            for _ in range(count):
                k,q=getstring(b,q);v,q=getstring(b,q)
                if not k or not re.fullmatch('[a-zA-Z0-9_-]+',k):raise ValueError()
                attrs.append((k,v))
        except (ValueError,struct.error,UnicodeError):continue
        if pair in attrs and p<=hit<q:candidates.append((p,q,attrs))
    assert len(candidates)==1,candidates
    p,q,attrs=candidates[0];assert attrs.count(pair)==1
    serialize=lambda kv:num(len(kv))+b''.join(string(k)+string(v) for k,v in kv)
    return serialize(attrs),serialize([t for t in attrs if t!=pair])
def replace(data,pairs):
    for before,after in pairs:
        pos(data,before);data=data.replace(before,after,1)
    return data
def compile_styles():
    root=current/'0012/ui/bin__'; doc='subtitletagview_game.uibinary';parts='subtitletagview_component_game.uibinary'
    original=(root/doc).read_bytes(); nodes=(root/parts).read_bytes()
    retail=(current/(UI+'subtitletagview.html')).read_bytes()
    style=lambda markup:re.search(rb'<style>(.*?)</style>',markup,re.S)[1]
    oldcss=style(retail);pos(original,string(oldcss))
    for k,label in [('classic','Classic Single Target'),('multi','Vanilla Multitarget'),('caites','Caites Multitarget')]:
        staged=A/'healthbar'/k;html=(staged/'files'/UI/'subtitletagview.html').read_bytes()
        css=style(html);out=replace(original,[(string(oldcss),string(css))])
        assert out.replace(string(css),string(oldcss),1)==original
        pairs=[]
        for seq in [('!cpp-none','cpp-head-up','fit-all','no-pickable'),('!cpp-none','headup-hp-stat-wrap')]:
            pairs.append((strings(seq),strings(seq[1:])))
        pairs.append(attributes(nodes,('selector-show-layer-list','#HPGauge')))
        if k!='classic':pairs.append(attributes(nodes,('selector-show-layer-list','.cpp-head-up')))
        edits=sorted((pos(nodes,old),old,new) for old,new in pairs)
        cur=0; rebuilt=bytearray();undo=[]
        for i,old,new in edits:
            assert cur<=i
            rebuilt+=nodes[cur:i];undo.append((len(rebuilt),old,new));rebuilt+=new;cur=i+len(old)
        rebuilt+=nodes[cur:];check=bytearray(rebuilt)
        for i,old,new in reversed(undo):
            assert check[i:i+len(new)]==new
            check[i:i+len(new)]=old
        assert check==nodes and rebuilt.count(string('#HPGauge'))==1
        assert (string('.cpp-head-up') in rebuilt)==(k=='classic')
        dest=staged/'files/0012/ui/bin__';dest.mkdir(exist_ok=True,parents=True)
        (dest/doc).write_bytes(out);(dest/parts).write_bytes(rebuilt)
        files={p.relative_to(staged).as_posix():p.read_bytes() for p in staged.rglob('*') if p.is_file()}
        if k!='classic':
            n=HERE/'native'
            files['Healthbar Multitarget.asi']=asi.read_bytes()
            files['LICENSE.txt']=(n/'LICENSE.txt').read_bytes()
        files['README.txt']=(HERE/'profiles/b26949954'/('README-'+k+'.txt')).read_bytes().replace(b'\r\n',b'\n')
        hashes=json.loads((HERE/'profiles/b26949954'/('members-'+k+'.json')).read_text())
        assert set(hashes)==set(files) and all(sha(raw)==hashes[name] for name,raw in files.items() if not name.endswith('.asi'))
        archive(D/('Healthbar - '+label+' v6.zip'),files)

if __name__=='__main__':
    cli=argparse.ArgumentParser(description=__doc__)
    for arg in ('current','parser','asi','output'):cli.add_argument('--'+arg,type=Path,required=True)
    args=cli.parse_args()
    current=args.current;asi=args.asi;D=args.output
    spec=importlib.util.spec_from_file_location('dmm_parser',args.parser)
    parser=importlib.util.module_from_spec(spec);spec.loader.exec_module(parser)
    with tempfile.TemporaryDirectory() as work:
        A=Path(work)
        stage()
        compile_styles()
