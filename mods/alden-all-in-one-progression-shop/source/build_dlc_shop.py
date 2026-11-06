"""Build the DLC-owner variant from the released 17.3.2 base shop.

The installed DLC supplies item definitions, effects, models and item text.
Only shop references, Copper price intents and the category helper change.
Older DLC input is a catalog reference, not a replacement for current DLC data.
All game-derived output stays in the caller's local build directory.
"""
import argparse
import copy
import json
import re
import shutil
import struct
import subprocess
import sys
import zipfile
from pathlib import Path

import build_runtime_shop as runtime
from original_shop import PREFIX, Table, split_stock
from ui_binary import Reader, strings

HERE = Path(__file__).resolve().parent
RECIPE = HERE.parents[2] / 'recipes/alden-dlc-b26361232'
VERSION = '17.4.1'
BASE_VERSION = '17.3.2'
BASE_SHA = '8b90164dc3beefacc10489686d8145cdf35e07d29e2479932b9c60ecd799f1ba'
BASE_ASI_SHA = '9983d998d98e8eb79b59ff344b7f76fbfb8b54d8d8fc7ac158914d613d2bac5d'
COUNTS = [841, 112, 111, 111, 114, 229, 74, 50, 40]
BIN = Path('0008/gamedata/binarystaticinfo__/bin')


def read_json(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def checked_replace(text, before, after):
    if text.count(before) != 1:
        raise ValueError('Unsupported native build input: ' + before[:80])
    return text.replace(before, after)


def load_dlc(args):
    for name, digest in read_json(RECIPE / 'input-hashes.json').items():
        assert runtime.sha((args.dlc_data / name).read_bytes()) == digest, name
    selected = read_json(RECIPE / 'catalog.json')['items']
    assert len(selected) == len({r['key'] for r in selected}) == 210
    assert [r['stock_index'] for r in selected] == list(range(607, 817))
    assert [sum(r['tab'] == tab for r in selected) for tab in range(1, 8)] == [18,18,40,19,104,11,0]
    selected += read_json(RECIPE / 'additions-17.4.1.json')['items']
    assert len(selected) == len({r['key'] for r in selected}) == 234
    assert [r['stock_index'] for r in selected] == list(range(607, 841))
    assert selected[210]['key'] == 1005625
    assert all(r['tab'] == 6 for r in selected[210:])
    assert not any(re.search(r'(?:LV|Level|Tier)[_-]?[23](?:$|_)', r['string_key'], re.I)
                   for r in selected if r['tab'] == 1)
    # A separate interpreter prevents the old/new native parser bindings from
    # sharing a module. Prove the cached DLC records match the actual old bytes.
    probe = '''import sys,json
from pathlib import Path
sys.path.insert(0,sys.argv[1])
import dmm_parser as p
base=Path(sys.argv[2]); body=(base/'iteminfo.dlc_01.staticinfobody').read_bytes()
rows=p.parse_table('iteminfo',body,(base/'iteminfo.dlc_01.staticinfoheader').read_bytes())
assert p.serialize_table('iteminfo',rows)==body
assert rows==json.loads(Path(sys.argv[3]).read_text(encoding='utf-8'))
print('PASS: original 1552 DLC records round-trip exactly')
'''
    subprocess.run([sys.executable, '-X', 'utf8', '-c', probe,
                    str(args.legacy_parser_dir.resolve()), str((args.dlc_data / BIN).resolve()),
                    str(args.dlc_items.resolve())], check=True)
    return selected, read_json(args.dlc_items)


def extend_stock(payload, selected, parser, dlc_data):
    bp, hp = [PREFIX + 'storeinfo.staticinfo' + suffix for suffix in ('body', 'header')]
    table = Table(payload[bp], payload[hp], 2)
    old = table.require(3101, 'Store_Her_General').data
    stock = split_stock(old, 607)
    assert len(stock[0]) == 127 and stock[0][114:] == bytes(13)
    additions = []
    for item in selected:
        row = bytearray(stock[0])
        for offset, value in [(18, item['stock']), (26, item['stock_index']),
                              (30, item['stock_index']), (43, item['key']), (102, item['key'])]:
            struct.pack_into('<I', row, offset, value)
        additions.append(bytes(row))
    prefix = bytearray(old[:81])
    for offset in (49,77):
        struct.pack_into('<I', prefix, offset, COUNTS[0])
    merged = bytes(prefix) + b''.join(stock + additions) + old[-10:]
    updated = table.replace({3101: merged})
    assert split_stock(merged, COUNTS[0])[:607] == stock
    before = parser.parse_table('storeinfo', table.body, table.header)
    after = parser.parse_table('storeinfo', updated.body, updated.header)
    expected = copy.deepcopy(before)
    original = next(r for r in expected if r['key'] == 3101)
    actual = next(r for r in after if r['key'] == 3101)
    assert actual['stock_data_list'][:607] == original['stock_data_list']
    original['buyable_stock_count'] = COUNTS[0]
    original['stock_data_list'] = actual['stock_data_list']
    assert after == expected
    for row, item in zip(actual['stock_data_list'][607:], selected, strict=True):
        assert row['raw_c'] == item['stock']
        assert row['raw_d'] == row['raw_e'] == item['stock_index']
    for row, item in zip(additions, selected, strict=True):
        assert struct.unpack_from('<I', row, 43)[0] == struct.unpack_from('<I', row, 102)[0] == item['key']
    dlc_stores = parser.parse_table('storeinfo',
        (dlc_data / BIN / 'storeinfo.dlc_01.staticinfobody').read_bytes(),
        (dlc_data / BIN / 'storeinfo.dlc_01.staticinfoheader').read_bytes())
    assert not any(r['key'] == 3101 for r in dlc_stores)
    payload[bp], payload[hp] = updated.body, updated.header
    return dict(existing_607_offer_bytes_preserved=True, other_base_stores_preserved=len(before)-1,
                reference_dlc_stores=len(dlc_stores), reference_dlc_overrides_alden=False)


def price_intents(selected, dlc_rows):
    by_key = {r['key']:r for r in dlc_rows}
    result = []
    for offer in selected:
        row = by_key[offer['key']]
        assert row['string_key'] == offer['string_key'] and not row['is_blocked']
        lists = [('price_list', row['price_list'])]
        lists += [(f'enchant_data_list[{i}].buy_price_list', e['buy_price_list'])
                  for i,e in enumerate(row['enchant_data_list'] or []) if e['buy_price_list']]
        for field, prices in lists:
            copper, = [i for i,p in enumerate(prices) if p['key'] == 1]
            assert prices[copper]['price']['item_info_wrapper'] == 1
            assert prices[copper]['price']['sym_no'] == 0
            result.append(dict(entry=row['string_key'],key=row['key'],
                               field=f'{field}[{copper}].price.price',op='set',new=1))
    assert len(result) == 252
    return result


def validate_prices(parser, current_data, dlc_rows, selected, base_intents, added):
    body = (current_data / BIN / 'iteminfo.staticinfobody').read_bytes()
    base = parser.parse_table('iteminfo', body, None)
    assert len(base) == 7069 and parser.serialize_table('iteminfo', base) == body
    base_ids = {r['key'] for r in base}
    dlc_ids = {r['key'] for r in dlc_rows}
    selected_ids = {r['key'] for r in selected}
    assert not selected_ids & base_ids
    assert not {i['key'] for i in base_intents} & dlc_ids
    # This is a validation model of base + old DLC overrides. No resulting
    # ItemInfo bytes are shipped. DMM patches the user's installed tables.
    effective = {r['key']:r for r in base}
    effective.update({r['key']:r for r in dlc_rows})
    model = parser.serialize_table('iteminfo', list(effective.values()))
    baseline = parser.parse_table('iteminfo', model, None)
    # Only the two schema additions since the reference DLC build are defaulted
    # by the parser for the local model. Every existing DLC field must survive.
    normalized = {r['key']:r for r in baseline}
    for original in dlc_rows:
        actual = normalized[original['key']]
        for key,value in original.items():
            assert actual[key] == value, (original['key'],key)
        extras = set(actual) - set(original)
        assert extras <= {'sound_socket_name_on_max_charged','sound_name_on_max_charged'}
        assert all(actual[k] == '' for k in extras)
    def apply(raw, intents):
        out = parser.apply_intents('iteminfo', raw, None, intents)
        assert len(out['outcomes']) == len(intents)
        assert all(r['status'] == 'applied' for r in out['outcomes'])
        return out['body']
    all_intents = base_intents + added
    expected = copy.deepcopy(baseline)
    normal_ids = {r['key'] for r in base_intents}
    for row in expected:
        if row['key'] in normal_ids:
            for price in row['price_list']:
                price['price']['price'] = 1
        if row['key'] in selected_ids:
            for prices in [row['price_list']] + [e['buy_price_list'] for e in row['enchant_data_list'] or []]:
                for price in prices:
                    if price['key'] == 1:
                        price['price']['price'] = 1
    patched = apply(model, all_intents)
    assert parser.parse_table('iteminfo', patched, None) == expected
    assert apply(patched, all_intents) == patched
    assert apply(apply(model, base_intents), added) == apply(apply(model, added), base_intents) == patched
    return dict(current_base_items=len(base), reference_dlc_items=len(dlc_rows),
                reference_dlc_overrides=len(base_ids & dlc_ids), modeled_effective_items=len(baseline),
                base_price_intents=len(base_intents), dlc_copper_intents=len(added),
                modeled_price_effects_exact=True, repeat_application_identical=True,
                original_item_effects_and_use_actions_preserved=True,
                model_scope='Current base game plus 2.04.05 DLC records. Not a current DLC extraction.')


def update_counts(data):
    r = Reader(data)
    r.pos = data.index(strings(['alden-category-headers','inherit-no-pickable']))
    headers = r.node()
    assert len(headers.children) == 9
    for n,child in enumerate(headers.children):
        label, = [node for node in child.walk() if node.id == 'AldenCategoryCount'+str(n)]
        assert label.optional[2] == [str([607,94,93,71,95,125,39,50,40][n])]
        label.optional[2] = [str(COUNTS[n])]
    return data[:headers.start] + headers.encode() + data[headers.end:]


def build_native(args, selected):
    native = args.output / 'native-build'
    shutil.copytree(args.base_build / 'native-build', native, dirs_exist_ok=True,
                    ignore=shutil.ignore_patterns('out', '*.log'))
    def edit(name, before, after):
        file = native / name
        file.write_text(checked_replace(file.read_text(encoding='utf-8'),before,after),encoding='utf-8')
    catalog = 'inline constexpr CatalogEntry DlcItems[] = {\n' + ''.join(
        '    {%du,%d},\n' % (i['key'],i['tab']) for i in selected) + '};\n'
    edit('catalog_generated.hpp','inline constexpr std::uint32_t OfferCount = ModOfferCount + OriginalOfferCount;',
        catalog+'inline constexpr std::uint32_t DlcStart = ModOfferCount + OriginalOfferCount;\n'
        +'inline constexpr std::uint32_t OfferCount = DlcStart + sizeof(DlcItems)/sizeof(DlcItems[0]);')
    edit('filter_core.hpp','return original<OriginalOfferCount && OriginalItems[original]==row.item ? 8 : 255;',
        'if(original<OriginalOfferCount) return OriginalItems[original]==row.item ? 8 : 255;\n'
        +'    if(row.index>=OfferCount) return 255;\n'
        +'    const auto& item=DlcItems[row.index-DlcStart];\n'
        +'    return item.id==row.item ? item.tab : 255;')
    edit('native_tests.cpp','alden::OriginalItems[i-alden::ModOfferCount];',
        'i<alden::DlcStart ? alden::OriginalItems[i-alden::ModOfferCount] : alden::DlcItems[i-alden::DlcStart].id;')
    edit('native_tests.cpp','607,94,93,71,95,125,39,50,40',','.join(map(str,COUNTS)))
    edit('native_tests.cpp','+0x100)==39,',f'+0x100)=={COUNTS[6]},')
    edit('native_tests.cpp','    Row one{1,Catalog[0].id,0};', '''    fixture::setup();states[0].tab=6;
    put(fixture::s()+0x188,DlcStart);count_hook(fixture::s(),DlcStart);
    check(get<unsigned>(fixture::s()+0x100)==39 && !states[0].fault,
          "missing DLC offers preserve the base catalog and sparse stock indices");
    fixture::setup();states[0].tab=1;
    put(reinterpret_cast<Ptr>(fixture::items[DlcStart].data())+8,0xffffffffu);
    count_hook(fixture::s(),OfferCount);
    check(states[0].fault && fixture::pointers==fixture::allPointers,
          "unexpected DLC item fails atomically without rewriting purchase rows");
    Row one{1,Catalog[0].id,0};''')
    edit('version_generated.h','17,3,2,0','17,4,1,0')
    edit('version_generated.h','17.3.2\\0',VERSION+'\\0')
    fixtures = native / 'fixtures'
    expected = []
    for name, token in [('vanilla','Vanilla'),('better','Better')]:
        path = fixtures / (name+'_expected.bin')
        before = path.read_bytes()
        after = update_counts(before)
        path.write_bytes(after)
        edit('runtime_generated.hpp',token+'OutputHash[]="'+runtime.sha(before)+'"',
             token+'OutputHash[]="'+runtime.sha(after)+'"')
        expected.append((before,after))
    vanilla = (fixtures/'vanilla.bin').read_bytes()
    old_edits = runtime.make_edits(expected[0][0],vanilla)
    new_edits = runtime.make_edits(expected[0][1],vanilla)
    for n,(old,new) in enumerate(zip(old_edits,new_edits,strict=True)):
        assert old[:2] == new[:2]
        if old[2] != new[2]:
            edit('runtime_generated.hpp',runtime.array('After'+str(n),old[2]),runtime.array('After'+str(n),new[2]))
    for name,(_,want) in zip(['vanilla','better'],expected):
        assert runtime.apply_edits((fixtures/(name+'.bin')).read_bytes(),new_edits) == want
    edit('runtime_generated.hpp','AddedSize='+str(len(expected[0][0])-len(vanilla)),
         'AddedSize='+str(len(expected[0][1])-len(vanilla)))
    edit('runtime_generated.hpp','Alden Category Tabs 17.3.2 /','Alden Category Tabs '+VERSION+' DLC /')
    subprocess.run(['cmd.exe','/d','/c','build_runtime.cmd'],cwd=native,check=True)
    return (native/'out/Alden Category Tabs.asi').read_bytes()


def build(args):
    args.output.mkdir(parents=True,exist_ok=True)
    selected,dlc_rows = load_dlc(args)
    sys.path.insert(0,str(args.parser_dir.resolve()))
    import dmm_parser as parser
    base_package = args.base_build/'Alden_Shop.zip'
    assert runtime.sha(base_package.read_bytes()) == BASE_SHA
    assert runtime.sha(args.game_exe.read_bytes()) == runtime.EXE_SHA
    assert runtime.PROFILE['steam_build'] == '26448710'
    with zipfile.ZipFile(base_package) as z:
        payload = {p:z.read(p) for p in z.namelist()}
    assert runtime.sha(payload['Alden Category Tabs.asi']) == BASE_ASI_SHA
    assert (args.base_build/'native-build/out/Alden Category Tabs.asi').read_bytes() == payload['Alden Category Tabs.asi']
    assert json.loads(payload['manifest.json'])['version'] == BASE_VERSION
    previous_files = {n:v for n,v in payload.items() if n.startswith('files/')}
    prices = json.loads(payload['Preise.json'])
    base_intents = prices['targets'][0]['intents']
    assert prices['targets'][0]['file'] == 'iteminfo.pabgb' and len(base_intents) == 706
    added = price_intents(selected,dlc_rows)
    report = validate_prices(parser,args.current_data,dlc_rows,selected,base_intents,added)
    report.update(extend_stock(payload,selected,parser,args.dlc_data))
    prices['targets'][0]['intents'] += added
    prices['modinfo'].update(title='Alden Shop DLC - 1 Copper',version=VERSION,requires_dlc=True,
        description='801 mod offers at 1 Copper, including Natural Pearl and required DLC ingredients. Requires Charting the Unknown. Use with Alden Shop DLC and its matching category ASI.')
    payload['Preise.json'] = json.dumps(prices,indent=2,ensure_ascii=False).encode('utf-8')
    payload['Alden Category Tabs.asi'] = build_native(args,selected)
    manifest = json.loads(payload['manifest.json'])
    manifest.update(id='AldenShopDLC',title='Alden Shop DLC',version=VERSION,requires_dlc=True,
        description='DLC community build for Steam 26448710. 801 mod offers plus 40 original offers in nine localized tabs. Adds 24 missing materials and ingredients. Requires Charting the Unknown. Community reported 17.4.0 shop working; new items await confirmation.')
    payload['manifest.json'] = json.dumps(manifest,indent=2,ensure_ascii=False).encode('utf-8')
    payload['README.txt'] = runtime.crlf((RECIPE/'PACKAGE_README-17.4.1.txt').read_text(encoding='utf-8'))
    changed_game_files = [n for n,v in previous_files.items() if payload[n] != v]
    assert set(changed_game_files) == {PREFIX+'storeinfo.staticinfobody',PREFIX+'storeinfo.staticinfoheader'}
    assert {n for n in payload if n.startswith('files/')} == set(previous_files)
    assert len(previous_files) == 20
    with zipfile.ZipFile(args.better_inventory) as z:
        bi = {p.partition('/files/')[2].partition('/')[2].lower() for p in z.namelist() if '/files/' in p}
    own = {p.split('/',2)[2].lower() for p in previous_files}
    assert not own & bi
    assert not any('iteminfo' in p or 'item.paloc' in p or '.dlc_01.' in p or p.endswith('.dds') for p in own)
    package = args.output/'Alden_Shop_DLC.zip'
    with zipfile.ZipFile(package,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for path,data in sorted(payload.items()):
            info = zipfile.ZipInfo(path,date_time=(2026,11,6,0,0,0))
            info.compress_type = zipfile.ZIP_DEFLATED
            z.writestr(info,data,compress_type=zipfile.ZIP_DEFLATED,compresslevel=9)
    with zipfile.ZipFile(package) as z:
        assert z.testzip() is None and all(z.read(n)==v for n,v in payload.items())
    (args.output/'Alden Category Tabs.asi').write_bytes(payload['Alden Category Tabs.asi'])
    (args.output/'README.txt').write_bytes(payload['README.txt'])
    report.update(version=VERSION,steam_build='26448710',exe_sha256=runtime.EXE_SHA,
        reference_dlc_build='26361232 / 2.04.05',normal_package_sha256=BASE_SHA,
        requires_dlc=True,dlc_offers=234,mod_offers=801,total_offers=841,distinct_items=832,
        category_counts=COUNTS,game_payload_files=20,changed_game_files=changed_game_files,
        shared_better_inventory_paths=[],dlc_item_definitions_or_assets_packaged=False,
        native_category_and_ui_regressions='PASS',current_dlc_data_available=False,
        dmm_mount_tested=False,in_game_tested=False,published=False,
        previous_version_feedback={'version':'17.4.0','shop_reported_working':True,
                                   'source':'Community feedback relayed by author',
                                   'full_dlc_validation':False},
        package_sha256=runtime.sha(package.read_bytes()),
        asi_sha256=runtime.sha(payload['Alden Category Tabs.asi']))
    (args.output/'VALIDATION.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,indent=2),flush=True)


if __name__ == '__main__':
    cli = argparse.ArgumentParser(description=__doc__)
    for name in ['base-build','game-exe','current-data','dlc-data','dlc-items',
                 'legacy-parser-dir','parser-dir','better-inventory','output']:
        cli.add_argument('--'+name,type=Path,required=True)
    build(cli.parse_args())
