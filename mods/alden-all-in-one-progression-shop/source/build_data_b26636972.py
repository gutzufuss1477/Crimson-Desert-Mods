"""Rebase the two exact published catalogs; do not import Alfred work in progress."""
from pathlib import Path
import sys,json,struct,zipfile,hashlib,importlib.util,copy,collections
import lz4.block
import argparse
HERE=Path(__file__).resolve().parent
cli=argparse.ArgumentParser(description=__doc__)
for name in ('inputs','parser','previous-regular','previous-dlc','output'):cli.add_argument('--'+name,type=Path,required=True)
args=cli.parse_args();C=args.inputs.resolve();O=args.output.resolve();A=O
O.mkdir(parents=True,exist_ok=True)
S=HERE.parents[2]
expected=json.loads((HERE/'alden-b26636972-inputs.json').read_text())
for path,digest in expected['inputs'].items():
    assert hashlib.sha256((C/path).read_bytes()).hexdigest()==digest,('Unsupported input',path)
archives={20198:args.previous_regular,20197:args.previous_dlc}
for fid,path in archives.items():assert hashlib.sha256(path.read_bytes()).hexdigest()==expected['previous_archives'][str(fid)]

sys.path.insert(0,str(S/'tooling'));from crimson_mod_tools.staticinfo import Table
binary=args.parser.resolve()
spec=importlib.util.spec_from_file_location('dmm_parser',binary);parser=importlib.util.module_from_spec(spec);spec.loader.exec_module(parser)
P='files/0008/gamedata/binarystaticinfo__/bin/'
removed={1005722,1005738,1005801}
sha=lambda x:hashlib.sha256(x).hexdigest()
def source(name):return [(C/'current'/f'0008/gamedata/binarystaticinfo__/bin/{name}.staticinfo{s}').read_bytes() for s in ('body','header')]
def indexed(body,header):
    count=struct.unpack_from('<H',header)[0];rows=[struct.unpack_from('<II',header,2+8*i) for i in range(count)]
    return {key:body[start:rows[i+1][1] if i+1<count else len(body)] for i,(key,start) in enumerate(rows)}
def stock_rows(data,count):
    marker=struct.pack('<HQQ',3101,1000000,1000000)
    offsets=[i for i in range(len(data)) if data.startswith(marker,i)]
    assert len(offsets)==count and offsets[0]==81,(len(offsets),count,offsets[:2])
    return [data[a:b] for a,b in zip(offsets,offsets[1:]+[len(data)-10])]
base=Table(*source('storeinfo'),2);native=base.require(3101,'Store_Her_General').data
base_local=source('localstringinfo');base_labels=parser.parse_table('localstringinfo',*base_local)
current_keys=set(indexed(*base_local));report=[]
for variant,fid,version in [('regular',20198,'17.3.7'),('dlc',20197,'17.4.6')]:
    with zipfile.ZipFile(archives[fid]) as z:payload={n:z.read(n) for n in z.namelist() if not n.endswith('/') and not n.endswith('.asi')}
    old=Table(payload[P+'storeinfo.staticinfobody'],payload[P+'storeinfo.staticinfoheader'],2)
    alden=next(x for x in parser.parse_table('storeinfo',old.body,old.header) if x['key']==3101)
    offers=alden['stock_data_list'];raw=old.require(3101,'Store_Her_General').data
    rows=stock_rows(raw,len(offers));updated=[];kept=[];saved=[]
    for row,offer in zip(rows,offers):
        item=offer['value']['payload']['body'];assert isinstance(item,int) and offer['value']['disc']==0
        assert struct.unpack_from('<I',row,43)[0]==item and struct.unpack_from('<I',row,102)[0]==item
        if item in removed:continue
        # New optional field precedes the discriminated item payload. Its default
        # is absent (00), independently attested in all 40 current original rows.
        updated.append(row[:102]+b'\0'+row[102:]);kept.append(offer)
        saved.append(dict(item=item,stock_index=offer['raw_d'],save_index=offer['raw_e']))
    assert len(offers)-len(kept)==3
    assert raw[:49]==native[:49] and raw[53:77]==native[53:77] and raw[-10:]==native[-10:]
    headerpart=raw[:49]+struct.pack('<I',len(updated))+raw[53:77]
    newrecord=headerpart+struct.pack('<I',len(updated))+b''.join(updated)+raw[-10:]
    newtable=base.replace({3101:newrecord})
    assert all(newtable.by_key[k].data==r.data for k,r in base.by_key.items() if k!=3101)
    # Exact inverse and the independent legacy parser prove every retained offer
    # and persistent stock key survived the wire-format insertion unchanged.
    inverse=headerpart+struct.pack('<I',len(kept))+b''.join(r[:102]+r[103:] for r in updated)+raw[-10:]
    inverse_table=old.replace({3101:inverse})
    decoded=next(x for x in parser.parse_table('storeinfo',inverse_table.body,inverse_table.header) if x['key']==3101)
    expected=copy.deepcopy(alden);expected['stock_data_list']=kept;expected['buyable_stock_count']=len(kept);assert decoded==expected
    native_rows=stock_rows(native,40)
    for i in range(40):
        row=rows[567+i];current=native_rows[i];candidate=bytearray(row[:102]+b'\0'+row[102:])
        candidate[26:34]=current[26:34] # persisted stock/save indices only
        assert bytes(candidate)==current,('original offer differs',i)
    payload[P+'storeinfo.staticinfobody']=newtable.body;payload[P+'storeinfo.staticinfoheader']=newtable.header
    local=parser.parse_table('localstringinfo',payload[P+'localstringinfo.staticinfobody'],payload[P+'localstringinfo.staticinfoheader'])
    labels=[r for r in local if r['key'] not in current_keys]
    own=indexed(payload[P+'localstringinfo.staticinfobody'],payload[P+'localstringinfo.staticinfoheader'])
    body=bytearray(base_local[0]);header=bytearray(struct.pack('<H',len(base_labels)+len(labels))+base_local[1][2:])
    for row in labels:header+=struct.pack('<II',row['key'],len(body));body+=own[row['key']]
    assert parser.parse_table('localstringinfo',bytes(body),bytes(header))==base_labels+labels
    payload[P+'localstringinfo.staticinfobody']=bytes(body);payload[P+'localstringinfo.staticinfoheader']=bytes(header)
    label_keys={str(r['game_string']['index']) for r in labels};languages=[]
    for path,oldbytes in list(payload.items()):
        if not path.endswith('/localstringinfo.paloc'):continue
        nativebytes=(C/'current'/path.removeprefix('files/')).read_bytes();entries=parser.parse_paloc_bytes(nativebytes)
        custom=[r for r in parser.parse_paloc_bytes(oldbytes) if r['string_key'] in label_keys]
        assert len(custom)==len(labels) and not label_keys.intersection(r['string_key'] for r in entries)
        rawloc=parser.serialize_paloc(entries+custom);comp=lz4.block.compress(rawloc,store_size=False)
        paloc=bytearray(nativebytes[:512]);struct.pack_into('<II',paloc,9,len(comp),len(rawloc));paloc+=comp
        assert parser.parse_paloc_bytes(bytes(paloc))==entries+custom
        payload[path]=bytes(paloc);languages.append(path.split('/')[-2])
    prices=json.loads(payload['Preise.json']);n_before=sum(len(t['intents']) for t in prices['targets'])
    for target in prices['targets']:
        target['intents']=[i for i in target['intents'] if int(i.get('record_key',i.get('key',-1))) not in removed]
    n_after=sum(len(t['intents']) for t in prices['targets']);assert n_before-n_after==3,(n_before,n_after,prices['targets'][0]['intents'][0])
    effective={}
    for kind in ['base']+(['dlc'] if variant=='dlc' else []):
        suffix='.dlc_01' if kind=='dlc' else '';root=C/f'raw/{kind}/0008/gamedata/binarystaticinfo__/bin/iteminfo{suffix}'
        effective.update({r['key']:r for r in parser.parse_table('iteminfo',*[Path(str(root)+'.staticinfo'+s).read_bytes() for s in ('body','header')])})
    assert all(r['value']['payload']['body'] in effective for r in kept)
    model=parser.serialize_table('iteminfo',list(effective.values()));applied=parser.apply_intents('iteminfo',model,None,prices['targets'][0]['intents'])
    assert len(applied['outcomes'])==n_after and all(r['status']=='applied' for r in applied['outcomes']),collections.Counter(r['status'] for r in applied['outcomes'])
    prices['modinfo']['version']=version;prices['modinfo']['description']=f'Alden catalog prices for Steam 26636972. {len(kept)} offers; matching package and ASI required. Use matching current native helper.'
    payload['Preise.json']=json.dumps(prices,ensure_ascii=False,indent=2).encode()
    manifest=json.loads(payload['manifest.json']);manifest.update(version=version,description=f'Steam 26636972 release. {len(kept)} offers. Current original merchants and localization preserved. Install the included ASI with the data package.')
    payload['manifest.json']=json.dumps(manifest,indent=2).encode()
    payload['README.txt']=(f'Alden Shop {version} - Steam build 26636972\n\nChoose ONE variant: Regular (base game) or DLC (requires Charting the Unknown).\nImport the ZIP into DMM. Enable the data mod and Alden Category Tabs in ASI Mods; mount and restart the game.\nRemove/disable older Alden packages and ASIs first. Keep your existing ASI loader.\nIncludes current merchant/localization data, updated native category tabs and prices.\nThree recipes removed from the game are no longer offered. All other persistent stock indices are preserved.\nNo Alfred development/test features are included.\nBuild output requires the matching native helper and validation before distribution.\n').encode()
    folder=O/variant;folder.mkdir(parents=True,exist_ok=True)
    for name,data in payload.items():
        path=folder/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data)
    row=dict(variant=variant,version=version,offers=len(kept),price_intents=n_after,custom_labels=len(labels),languages=len(languages),current_other_stores_preserved=len(base.records)-1,removed_item_keys=sorted(removed),saved_stock_keys=saved,stock_wire_roundtrip=True,original_40_offers_current=True,gameplay_tested=False)
    report.append(row);print({k:v for k,v in row.items() if k!='saved_stock_keys'},flush=True)
(A/'alden-data-validation.json').write_text(json.dumps(report,indent=2))
