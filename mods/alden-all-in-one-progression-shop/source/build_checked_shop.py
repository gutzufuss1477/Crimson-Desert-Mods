"""Build separate current-data Alden packages; never mounts or publishes.

Regular 17.3.4 adds two base-game ingredients. DLC 17.4.3 also places the
existing 234 DLC offers in a separate tenth tab. Existing stock IDs survive.
Requires DMM 4.7.4+ for the native DLC-vendor promotion fixed in DMM 4.7.3.
"""
import argparse
import copy
import ctypes
import json
import re
import shutil
import struct
import subprocess
import sys
import time
import zipfile
from pathlib import Path

import lz4.block
import build_runtime_shop as runtime
from original_shop import Table, PREFIX, split_stock
from ui_binary import Reader, strings

HERE = Path(__file__).resolve().parent
EXE_SHA = 'c94ad6c3ea54970a6d2da7a2719753210a7728a91698ed3405dd9125470ed98f'
EXTRA_ITEMS = (756518, 22008)  # Long Horn / Water, both base-game ingredients.
DLC_LABEL_KEY = 4098480238  # hashlittle('ui_aldencategory_dlc', 0xc5ede)
DLC_LABEL_INDEX = (DLC_LABEL_KEY << 32) | 0xb0
VARIANTS = {
    'regular': dict(version='17.3.4', previous='17.3.3', oldzip='Alden_Shop.zip',
                    zip='Alden_Shop_No_DLC.zip', oldoffers=607,
                    sha='24c8d5b5cbe4848b3985b1750ff7ea301b761b0a3886b07233db67b083ec0351',
                    counts=[609,94,93,71,95,125,41,50,40]),
    'dlc': dict(version='17.4.3', previous='17.4.2', oldzip='Alden_Shop_DLC.zip',
                zip='Alden_Shop_DLC.zip', oldoffers=841,
                sha='08f0ae50f8fc6c85bebd09a30c16b44a23aea97a3d3f64af3c35ba9effd8fce7',
                counts=[843,94,93,71,95,125,41,50,40,234]),
}


def load_json(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def replace_once(text, before, after):
    assert text.count(before) == 1, before[:100]
    return text.replace(before, after)


def ui(data, counts):
    r = Reader(data)
    r.pos = data.index(strings(['alden-category-headers','inherit-no-pickable']))
    headers = r.node()
    assert len(headers.children) == 9
    if len(counts) == 10:
        extra = copy.deepcopy(headers.children[-1])
        for node in extra.walk():
            assert node.names[0].endswith('8')
            node.names[0] = node.names[0][:-1] + '9'
            node.css = [s.replace('alden-tab-8','alden-tab-9') for s in node.css]
            if node.names[1]:
                assert node.names[1] == 'UI_AldenCategory_Original'
                node.names[1] = 'UI_AldenCategory_DLC'
        headers.children.append(extra)
    for index, child in enumerate(headers.children):
        counter, = [n for n in child.walk() if n.id == 'AldenCategoryCount'+str(index)]
        counter.optional[2] = [str(counts[index])]
    out = data[:headers.start] + headers.encode() + data[headers.end:]
    assert out.count(runtime.string('need-trigger-downonce-release')+runtime.string('True')) == 2
    return out


def append_dlc_label(payload, parser):
    bp,hp = [PREFIX+'localstringinfo.staticinfo'+s for s in ('body','header')]
    body,header = payload[bp],payload[hp]
    rows = parser.parse_table('localstringinfo',body,header)
    assert DLC_LABEL_KEY not in {r['key'] for r in rows}
    extra = copy.deepcopy(rows[-1]);extra['key'] = DLC_LABEL_KEY
    extra['game_string']['index'] = DLC_LABEL_INDEX
    extra['game_string']['default'] = str(DLC_LABEL_INDEX)
    payload[bp] = body + parser.serialize_table('localstringinfo',[extra])
    n, = struct.unpack_from('<H',header)
    payload[hp] = struct.pack('<H',n+1)+header[2:]+struct.pack('<II',DLC_LABEL_KEY,len(body))
    assert parser.parse_table('localstringinfo',payload[bp],payload[hp]) == rows+[extra]
    for name,data in list(payload.items()):
        if not name.endswith('/localstringinfo.paloc'): continue
        labels = parser.parse_paloc_bytes(data)
        added = dict(unk_id=11,string_key=str(DLC_LABEL_INDEX),string_value='DLC')
        assert added['string_key'] not in {r['string_key'] for r in labels}
        raw = parser.serialize_paloc(labels+[added])
        packed = lz4.block.compress(raw,store_size=False)
        head = bytearray(data[:512]);struct.pack_into('<II',head,9,len(packed),len(raw))
        payload[name] = bytes(head)+packed
        assert parser.parse_paloc_bytes(payload[name]) == labels+[added]


def build_native(args, variant):
    native = args.output/'native-build'
    shutil.copytree(args.previous_build/'native-build',native,dirs_exist_ok=True,
                    ignore=shutil.ignore_patterns('out','*.log'))
    def edit(name, before, after):
        p = native/name;p.write_text(replace_once(p.read_text(),before,after))
    header = (native/'catalog_generated.hpp').read_text()
    header = replace_once(header,'inline constexpr std::uint32_t OfferCount =',
                          'inline constexpr std::uint32_t ExtraStart =')
    header += '\nnamespace alden {\ninline constexpr std::uint32_t ExtraItems[] = {756518,22008};\n'
    header += 'inline constexpr std::uint32_t OfferCount = ExtraStart + 2;\n}\n'
    if args.variant == 'dlc':
        header = replace_once(header,'TabCount = 9','TabCount = 10')
    (native/'catalog_generated.hpp').write_text(header)
    edit('filter_core.hpp','    if(row.index<ModOfferCount)',
        '    if(row.index>=ExtraStart) return row.index<OfferCount && ExtraItems[row.index-ExtraStart]==row.item ? 6 : 255;\n'
        '    if(row.index<ModOfferCount)')
    if args.variant == 'dlc':
        edit('filter_core.hpp','return item.id==row.item ? item.tab : 255;',
             'return item.id==row.item ? 9 : 255;')
    tests = (native/'native_tests.cpp').read_text()
    tests = replace_once(tests,'const auto id=i<alden::ModOfferCount ?',
                         'const auto id=i>=alden::ExtraStart ? alden::ExtraItems[i-alden::ExtraStart] : i<alden::ModOfferCount ?')
    tests = re.sub(r'constexpr unsigned expected\[\]=\{[^}]+\};',
                   'constexpr unsigned expected[]={'+','.join(map(str,variant['counts']))+'};',tests)
    tests = re.sub(r'==(?:39|74),"material tab filters', '==41,"material tab filters', tests)
    extra_test = '''    fixture::setup();states[0].tab=6;
    count_hook(fixture::s(),OfferCount);
    for(unsigned i=ExtraStart;i<OfferCount;++i) {
        check(std::find(fixture::pointers.begin(),fixture::pointers.begin()+41,fixture::allPointers[i])!=fixture::pointers.begin()+41,
              "Long Horn and Water keep their own purchase rows in Materials");
    }
    fixture::setup();states[0].tab=6;
    put(fixture::allPointers[ExtraStart]+0x28,0xffffffffu);
    count_hook(fixture::s(),OfferCount);
    check(states[0].fault && fixture::pointers==fixture::allPointers,"invalid ingredient index fails atomically");
'''
    tests = replace_once(tests,'    Row one{1,Catalog[0].id,0};',extra_test+'    Row one{1,Catalog[0].id,0};')
    tests = tests.replace('nine tabs,',str(len(variant['counts']))+' tabs,')
    (native/'native_tests.cpp').write_text(tests)
    edits = None
    generated = (native/'runtime_generated.hpp').read_text()
    for name,token in [('vanilla','Vanilla'),('better','Better')]:
        old = (native/f'fixtures/{name}_expected.bin').read_bytes()
        new = ui(old,variant['counts'])
        source = (native/f'fixtures/{name}.bin').read_bytes()
        updated = runtime.make_edits(new,source)
        if edits is None: edits = updated
        else: assert updated == edits
        assert runtime.apply_edits(source,updated) == new
        # Ensure the native grid and existing dependencies are byte-identical.
        def grid(blob):
            reader=Reader(blob);reader.pos=blob.index(strings(['two-column-list-section-wrap']));return reader.node().encode()
        assert grid(old)==grid(new) and runtime.stylesheet_list(old)[2]==runtime.stylesheet_list(new)[2]
        generated = replace_once(generated,token+'OutputHash[]="'+runtime.sha(old)+'"',
                                  token+'OutputHash[]="'+runtime.sha(new)+'"')
        (native/f'fixtures/{name}_expected.bin').write_bytes(new)
    for index,(_,_,after) in enumerate(edits):
        generated,n = re.subn(r'inline constexpr unsigned char After'+str(index)+r'\[\] = \{.*?\};',
                             lambda _:runtime.array('After'+str(index),after),generated,flags=re.S)
        assert n==1
    generated = re.sub(r'AddedSize=\d+;',f'AddedSize={sum(len(a)-len(b) for _,b,a in edits)};',generated)
    generated = generated.replace('Alden Category Tabs '+variant['previous']+' /','Alden Category Tabs '+variant['version']+' /')
    (native/'runtime_generated.hpp').write_text(generated)
    for name in ['native_tabs.cpp','runtime_ui.hpp']:
        text=(native/name).read_text().replace('nine tabs including Original','category tabs including Original')
        text=text.replace('nine categories',str(len(variant['counts']))+' categories')
        (native/name).write_text(text)
    version=variant['version']
    (native/'version_generated.h').write_text('#pragma once\n#define ALDEN_FILE_VERSION '+version.replace('.',',')+',0\n#define ALDEN_FILE_FLAGS 0\n#define ALDEN_VERSION_TEXT "'+version+'\\0"\n')
    subprocess.run(['cmd.exe','/d','/c','build_runtime.cmd'],cwd=native,check=True)
    asi=native/'out/Alden Category Tabs.asi'
    start=time.time_ns();ctypes.WinDLL(str(asi.resolve()));log=Path(str(asi)+'.log')
    for _ in range(40):
        if log.exists() and log.stat().st_mtime_ns>=start and 'REFUSED: wrong process' in log.read_text(errors='replace'):break
        time.sleep(.1)
    else:raise AssertionError('Wrong-process guard failed')
    return asi.read_bytes()


def build(args):
    variant=VARIANTS[args.variant];args.output.mkdir(parents=True,exist_ok=True)
    assert runtime.sha(args.game_exe.read_bytes())==EXE_SHA
    previous=args.previous_build/variant['oldzip']
    assert runtime.sha(previous.read_bytes())==variant['sha']
    sys.path.insert(0,str(args.parser_dir.resolve()));import dmm_parser as parser
    with zipfile.ZipFile(previous) as z: payload={n:z.read(n) for n in z.namelist()}
    from build_release_hotfix import rebase_data
    report=rebase_data(payload,args.current_data,parser)
    bp,hp=[PREFIX+'storeinfo.staticinfo'+s for s in ('body','header')]
    table=Table(payload[bp],payload[hp],2);old=table.require(3101,'Store_Her_General').data
    stores=parser.parse_table('storeinfo',table.body,table.header)
    alden=copy.deepcopy(next(r for r in stores if r['key']==3101))
    assert len(alden['stock_data_list'])==variant['oldoffers']
    existing={r['value']['raw_q'] for r in alden['stock_data_list']}
    assert not existing.intersection(EXTRA_ITEMS)
    for index,key in enumerate(EXTRA_ITEMS,variant['oldoffers']):
        row=copy.deepcopy(alden['stock_data_list'][0]);row['raw_c']=999
        row['raw_d']=row['raw_e']=index;row['value']['payload']['body']=row['value']['raw_q']=key
        alden['stock_data_list'].append(row)
    alden['buyable_stock_count']=variant['counts'][0]
    rebuilt=table.replace({3101:parser.serialize_table('storeinfo',[alden])})
    assert split_stock(rebuilt.require(3101,'Store_Her_General').data,variant['counts'][0])[:variant['oldoffers']]==split_stock(old,variant['oldoffers'])
    expected=[alden if r['key']==3101 else r for r in stores]
    assert parser.parse_table('storeinfo',rebuilt.body,rebuilt.header)==expected
    payload[bp],payload[hp]=rebuilt.body,rebuilt.header
    itemdir=args.current_data/'0008/gamedata/binarystaticinfo__/bin'
    base=parser.parse_table('iteminfo',(itemdir/'iteminfo.staticinfobody').read_bytes(),(itemdir/'iteminfo.staticinfoheader').read_bytes())
    items={r['key']:r for r in base}
    assert items[756518]['string_key']=='Deer_Antler' and items[22008]['string_key']=='Water'
    patch=json.loads(payload['Preise.json']);old_intents=copy.deepcopy(patch['targets'])
    for key in EXTRA_ITEMS:
        item=items[key];assert not item['is_blocked']
        copper,=[i for i,row in enumerate(item['price_list']) if row['key']==1]
        patch['targets'][0]['intents'].append(dict(entry=item['string_key'],key=key,field=f'price_list[{copper}].price.price',op='set',new=1))
    assert patch['targets'][0]['intents'][:-2]==old_intents[0]['intents']
    if args.variant=='dlc':append_dlc_label(payload,parser)
    label='DLC' if args.variant=='dlc' else 'No DLC'
    patch['modinfo'].update(version=variant['version'],title='Alden Shop '+label+' - 1 Copper',
        description='DMM 4.7.4+. Existing prices retained; adds Long Horn and Water for 1 Copper.')
    payload['Preise.json']=json.dumps(patch,ensure_ascii=False,indent=2).encode()
    manifest=json.loads(payload['manifest.json']);manifest.update(version=variant['version'],title='Alden Shop '+label,
        description=f'Steam 26480954; DMM 4.7.4+. {variant["counts"][0]} offers in {len(variant["counts"])} categories. Adds Long Horn and Water. '+('234 DLC offers in their own DLC tab. Requires installed Charting the Unknown.' if args.variant=='dlc' else 'No DLC required.'))
    payload['manifest.json']=json.dumps(manifest,ensure_ascii=False,indent=2).encode()
    payload['Alden Category Tabs.asi']=build_native(args,variant)
    payload['README.txt']=runtime.crlf((HERE/('CHECKED_'+args.variant.upper()+'_README.txt')).read_text(encoding='utf-8'))
    assert sum(n.startswith('files/') for n in payload)==20
    assert not any('.dlc_01.' in n or 'iteminfo.' in n or n.endswith('.dds') for n in payload)
    dest=args.output/variant['zip']
    with zipfile.ZipFile(dest,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for name,data in sorted(payload.items()):
            info=zipfile.ZipInfo(name,(2026,11,7,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED
            z.writestr(info,data,compress_type=zipfile.ZIP_DEFLATED,compresslevel=9)
    with zipfile.ZipFile(dest) as z:assert z.testzip() is None
    for name in ['README.txt','Alden Category Tabs.asi']:(args.output/name).write_bytes(payload[name])
    report.update(version=variant['version'],variant=args.variant,steam_build='26480954',exe_sha256=EXE_SHA,
        previous_offer_count=variant['oldoffers'],previous_stock_rows_byte_identical=True,
        category_counts=variant['counts'],added_base_ingredients=list(EXTRA_ITEMS),ingredient_stock=999,ingredient_copper_price=1,
        price_intents=sum(len(t['intents']) for t in patch['targets']),
        package_sha256=runtime.sha(dest.read_bytes()),asi_sha256=runtime.sha(payload['Alden Category Tabs.asi']),
        native_regressions='PASS',runtime_ui_regressions='PASS',wrong_process_guard='PASS',
        minimum_dmm='4.7.4',in_game_tested=False,published=False,installed=False)
    report.pop('alden_store_bytes_unchanged',None)
    (args.output/'VALIDATION.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':
    cli=argparse.ArgumentParser(description=__doc__)
    cli.add_argument('--variant',choices=list(VARIANTS),required=True)
    for name in ['previous-build','current-data','parser-dir','game-exe','output']:
        cli.add_argument('--'+name,type=Path,required=True)
    build(cli.parse_args())
