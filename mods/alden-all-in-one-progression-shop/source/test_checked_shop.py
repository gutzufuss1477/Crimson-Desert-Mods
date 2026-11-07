"""Validate the separate Alden packages against explicitly supplied installed data.

Offline assertions only: DMM mounting and in-game display/purchases are not simulated.
"""
import argparse
import copy
import hashlib
import json
import struct
import sys
import zipfile
from pathlib import Path

from original_shop import PREFIX, Table, split_stock
from build_checked_shop import VARIANTS, EXTRA_ITEMS, DLC_LABEL_KEY


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def run(a):
    sys.path.insert(0, str(a.parser_dir.resolve()))
    import dmm_parser as p
    v = VARIANTS[a.variant]
    dlc = a.variant == 'dlc'
    def table(kind, name):
        root = a.data/'raw'/kind/'0008/gamedata/binarystaticinfo__/bin'
        suffix = '.dlc_01' if kind == 'dlc' else ''
        body = (root/(name+suffix+'.staticinfobody')).read_bytes()
        header = (root/(name+suffix+'.staticinfoheader')).read_bytes()
        rows = p.parse_table(name, body, header)
        assert p.serialize_table(name, rows) == body, (kind, name, 'roundtrip')
        return rows
    base_items = table('base', 'iteminfo')
    dlc_items = table('dlc', 'iteminfo')
    effective = {r['key']:r for r in base_items}
    if dlc:
        effective.update({r['key']:r for r in dlc_items})
    with zipfile.ZipFile(a.package) as z:
        assert z.testzip() is None
        payload = {n:z.read(n) for n in z.namelist()}
    with zipfile.ZipFile(a.previous_package) as z:
        previous = {n:z.read(n) for n in z.namelist()}
    assert hashlib.sha256(a.previous_package.read_bytes()).hexdigest() == v['sha']
    manifest = json.loads(payload['manifest.json'])
    prices = json.loads(payload['Preise.json'])
    assert manifest['version'] == prices['modinfo']['version'] == v['version']
    assert bool(manifest.get('requires_dlc')) == bool(prices['modinfo'].get('requires_dlc')) == dlc
    assert not any('.dlc_01.' in n or 'iteminfo.' in n or n.endswith('.dds') for n in payload)
    old = Table(previous[PREFIX+'storeinfo.staticinfobody'],previous[PREFIX+'storeinfo.staticinfoheader'],2)
    new = Table(payload[PREFIX+'storeinfo.staticinfobody'],payload[PREFIX+'storeinfo.staticinfoheader'],2)
    stock = split_stock(new.require(3101,'Store_Her_General').data,v['counts'][0])
    assert stock[:-2] == split_stock(old.require(3101,'Store_Her_General').data,v['oldoffers'])
    ids = [struct.unpack_from('<I',r,43)[0] for r in stock]
    assert ids[-2:] == list(EXTRA_ITEMS)
    for index,row in enumerate(stock[-2:],v['oldoffers']):
        assert struct.unpack_from('<I',row,18)[0] == 999
        assert struct.unpack_from('<II',row,26) == (index,index)
    assert set(ids) <= effective.keys()
    assert not any(effective[key]['is_blocked'] for key in ids)
    current_stores = table('base','storeinfo')
    result_stores = p.parse_table('storeinfo',new.body,new.header)
    assert [r for r in result_stores if r['key']!=3101] == [r for r in current_stores if r['key']!=3101]
    changes = prices['targets'][0]['intents']
    assert changes[:-2] == json.loads(previous['Preise.json'])['targets'][0]['intents']
    assert len(changes) == (960 if dlc else 708)
    rule, = read(a.equip_all)['targets'][0]['intents']
    steel = read(a.steelheart)['targets'][0]['intents']
    rows = list(effective.values())
    armor = [dict(key=r['key'],field=f'prefab_data_list[{i}].tribe_gender_list',op='set',new=[])
             for r in rows if r['equip_type_info'] in rule['match']['equip_type_info']
             for i in range(len(r['prefab_data_list']))]
    def apply(body, intents):
        result=p.apply_intents('iteminfo',body,None,intents)
        assert len(result['outcomes'])==len(intents)
        assert all(r['status']=='applied' for r in result['outcomes'])
        return result['body']
    body=p.serialize_table('iteminfo',rows)
    combined=apply(body,armor+changes+steel)
    assert combined==apply(body,steel+changes+armor)
    assert combined==apply(apply(body,armor),changes+steel)
    assert combined==apply(apply(body,changes+steel),armor)
    assert combined==apply(combined,armor+changes+steel)
    expected=copy.deepcopy(rows)
    legacy_base={r['key'] for r in changes[:706]}
    copper_only={r['key'] for r in changes[706:]}
    for row in expected:
        if row['equip_type_info'] in rule['match']['equip_type_info']:
            for prefab in row['prefab_data_list']:
                prefab['tribe_gender_list']=[]
        if row['key'] in legacy_base:
            for price in row['price_list']:
                price['price']['price']=1
        if row['key'] in copper_only:
            lists=[row['price_list']]
            if row['key'] not in EXTRA_ITEMS:
                lists += [e['buy_price_list'] for e in row['enchant_data_list'] or []]
            for prices_list in lists:
                for price in prices_list:
                    if price['key']==1:
                        price['price']['price']=1
        if row['key']==1000594:
            row['enchant_data_list'][0]['equip_buffs'][0]['level']=20
    assert p.parse_table('iteminfo',combined,None)==expected
    # DMM routes intents to separate base/DLC tables. Verify each subset too;
    # the union above is an offline composition model, not a DMM mount test.
    for source_rows in ([base_items,dlc_items] if dlc else [base_items]):
        keys={r['key'] for r in source_rows}
        applicable=[r for r in changes if r['key'] in keys]
        raw=p.serialize_table('iteminfo',source_rows)
        patched=apply(raw,applicable)
        assert apply(patched,applicable)==patched
    selected=[effective[key] for key in sorted(set(ids))]
    strings={r['key']:r for r in table('base','stringinfo')}
    if dlc:
        strings.update({r['key']:r for r in table('dlc','stringinfo')})
    texture_paths={r['path'] for r in read(a.data/'inventory.json')
                   if r['group']=='0012' and (dlc or r['kind']=='base')}
    icons=set()
    for row in selected:
        assert row['item_icon_list'], row['string_key']
        for icon in row['item_icon_list']:
            path='ui/texture/icon/'+strings[icon['icon_path']]['buffer'].lower()+'.dds'
            assert path in texture_paths,(row['string_key'],path)
            icons.add(path)
    names={}
    for base_path in sorted((a.data/'raw/base').glob('*/**/item.paloc')):
        lang=base_path.parent.name
        texts={r['string_key']:r['string_value'] for r in p.parse_paloc_bytes(base_path.read_bytes())}
        if dlc:
            rel=base_path.relative_to(a.data/'raw/base')
            dlc_path=a.data/'raw/dlc'/rel.with_name('item.dlc_01.paloc')
            texts.update({r['string_key']:r['string_value'] for r in p.parse_paloc_bytes(dlc_path.read_bytes())})
        for row in selected:
            for field in ['item_name','item_desc']:
                key=str(row[field]['index'])
                assert texts.get(key),(lang,row['string_key'],field,key)
        names[lang]={key:texts[str(effective[key]['item_name']['index'])] for key in EXTRA_ITEMS}
    assert len(names)==15
    assert list(names['eng'].values())==['Long Horn','Water']
    labels=p.parse_table('localstringinfo',payload[PREFIX+'localstringinfo.staticinfobody'],payload[PREFIX+'localstringinfo.staticinfoheader'])[-len(v['counts']):]
    assert len({r['key'] for r in labels})==len(v['counts'])
    dlc_loc_keys={r['key'] for r in table('dlc','localstringinfo')}
    assert not {r['key'] for r in labels}&dlc_loc_keys
    translations=read(Path(__file__).with_name('translations.json'))
    for name,data in payload.items():
        if name.endswith('/localstringinfo.paloc'):
            added=p.parse_paloc_bytes(data)[-len(labels):]
            assert [r['string_key'] for r in added]==[str(r['game_string']['index']) for r in labels]
            assert [r['string_value'] for r in added]==translations[name.split('/')[-2]]+(['DLC'] if dlc else [])
    books=[]
    if dlc:
        assert labels[-1]['key']==DLC_LABEL_KEY
        books=[r for r in dlc_items if r['string_key'].startswith('Recipe_Book') and not r['is_blocked']]
        assert len(books)==40 and {r['key'] for r in books}<=set(ids[607:841])
        knowledge={r['key']:r for kind in ['base','dlc'] for r in table(kind,'knowledgeinfo')}
        uses={r['key']:r for kind in ['base','dlc'] for r in table(kind,'itemuseinfo')}
        for book in books:
            for ref in book['item_use_info_list'] or []:
                assert ref in uses and not uses[ref]['is_blocked']
            rewards=[r['reward_knowledge_info'] for r in book['inspect_data_list'] if r['reward_knowledge_info']]
            assert rewards,book['string_key']
            assert all(key in knowledge and not knowledge[key]['is_blocked'] for key in rewards)
        assert any('Pearl' in effective[key]['string_key'] for key in ids[607:841])
    with zipfile.ZipFile(a.better_inventory) as z:
        bi_paths={n.lower().split('/files/',1)[-1] for n in z.namelist() if '/files/' in n.lower()}
        assert bi_paths, 'Better Inventory paths not recognized'
    our_paths={n.lower()[6:] for n in payload if n.startswith('files/')}
    assert not our_paths&bi_paths, our_paths&bi_paths
    report=dict(version=v['version'],offers=len(ids),distinct_items=len(set(ids)),
        current_data_names_and_descriptions_checked_in_languages=sorted(names),
        icon_asset_paths_resolved=len(icons),added_ingredients=names,
        current_dlc_book_unlock_references_validated=len(books),
        every_item_field_matches_independent_expected_result=True,
        equip_all_armor_and_steelheart_both_orders_and_repeat='PASS',
        base_and_dlc_price_intents_separately_validated=True,
        existing_stock_and_save_ids_preserved=True,other_439_merchants_unchanged=True,
        original_better_inventory_file_intersection=[],
        in_game_tested=False,dmm_mount_performed=False)
    a.report.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,ensure_ascii=False,indent=2))


if __name__=='__main__':
    cli=argparse.ArgumentParser(description=__doc__)
    cli.add_argument('--variant',choices=list(VARIANTS),required=True)
    for name in ['parser-dir','data','package','previous-package','equip-all','steelheart','better-inventory','report']:
        cli.add_argument('--'+name,type=Path,required=True)
    run(cli.parse_args())
