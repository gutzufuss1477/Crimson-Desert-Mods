"""Offline DLC package composition checks using explicit local game/mod inputs."""
import argparse
import copy
import hashlib
import json
import re
import struct
import sys
import zipfile
from pathlib import Path


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def run(args):
    sys.path.insert(0,str(args.parser_dir.resolve()))
    import dmm_parser as parser
    with zipfile.ZipFile(args.package) as z:
        payload = {p:z.read(p) for p in z.namelist()}
    patch = json.loads(payload['Preise.json'])
    manifest = json.loads(payload['manifest.json'])
    assert patch['modinfo']['requires_dlc'] is True and manifest['requires_dlc'] is True
    assert manifest['version'] == patch['modinfo']['version'] == args.expected_version
    intents = patch['targets'][0]['intents']
    assert len(intents) == 958 and len({i['key'] for i in intents}) == 801
    with zipfile.ZipFile(args.previous_package) as z:
        previous = {p:z.read(p) for p in z.namelist()}
    assert hashlib.sha256(args.previous_package.read_bytes()).hexdigest() == '6ccbaf8fbafe2974563d533a15801ae2168a11306acfd751566ae0b572e14fc8'
    old_intents = json.loads(previous['Preise.json'])['targets'][0]['intents']
    assert intents[:len(old_intents)] == old_intents and len(intents)-len(old_intents) == 24
    from original_shop import Table, split_stock, PREFIX
    table = Table(payload[PREFIX+'storeinfo.staticinfobody'],payload[PREFIX+'storeinfo.staticinfoheader'],2)
    old_table = Table(previous[PREFIX+'storeinfo.staticinfobody'],previous[PREFIX+'storeinfo.staticinfoheader'],2)
    stock = split_stock(table.require(3101,'Store_Her_General').data,841)
    prior = split_stock(old_table.require(3101,'Store_Her_General').data,817)
    assert stock[:817] == prior
    for r in old_table.records:
        if r.key != 3101:
            assert table.require(r.key,r.name).data == r.data
    stock_ids = [struct.unpack_from('<I',r,43)[0] for r in stock]
    for i,r in enumerate(stock[817:],817):
        assert struct.unpack_from('<I',r,18)[0] == 999
        assert struct.unpack_from('<II',r,26) == (i,i)
        assert struct.unpack_from('<I',r,43)[0] == struct.unpack_from('<I',r,102)[0]
    added_ids = {i['key'] for i in intents[len(old_intents):]}
    assert set(stock_ids[817:]) == added_ids
    assert added_ids == {
        1005625,1004791,1005500,1004796,1004790,1005499,1006531,1006536,
        1006535,1006537,1006533,1006532,1005457,1004795,1004794,1004792,
        1004788,1006190,1005874,1006917,1006918,1006919,1006920,1006921}
    catalog_source = (args.package.parent/'native-build/catalog_generated.hpp').read_text(encoding='utf-8')
    category = {int(key):int(tab) for key,tab in re.findall(r'\{\s*(\d+)u?,\s*(\d+)\s*\}',catalog_source)}
    assert all(category[key] == 6 for key in added_ids)
    dlc_books = {r['key'] for r in read(args.dlc_items)
                 if r['string_key'].startswith('Recipe_Book') and not r['is_blocked']}
    assert len(dlc_books) == 40 and dlc_books <= set(stock_ids)
    assert all(category[key] == 3 for key in dlc_books)
    book_offers = [key for i,key in enumerate(stock_ids) if not 567<=i<607 and category[key]==3]
    assert len(book_offers) == len(set(book_offers)) == 111
    rows = parser.parse_table('iteminfo',args.item_body.read_bytes(),None)
    effective = {r['key']:r for r in rows}
    effective.update({r['key']:r for r in read(args.dlc_items)})
    body = parser.serialize_table('iteminfo',list(effective.values()))
    items = parser.parse_table('iteminfo',body,None)
    armor_rule, = read(args.equip_all)['targets'][0]['intents']
    assert armor_rule['field'] == 'prefab_data_list[*].tribe_gender_list' and armor_rule['new'] == []
    armor = [dict(key=r['key'],field=f'prefab_data_list[{i}].tribe_gender_list',op='set',new=[])
             for r in items if r['equip_type_info'] in armor_rule['match']['equip_type_info']
             for i in range(len(r['prefab_data_list']))]
    steel = read(args.steelheart)['targets'][0]['intents']
    assert len(steel) == 1
    def apply(raw, changes):
        result = parser.apply_intents('iteminfo',raw,None,changes)
        assert len(result['outcomes']) == len(changes)
        assert all(o['status'] == 'applied' for o in result['outcomes'])
        return result['body']
    combined = apply(body,armor+intents+steel)
    assert apply(body,steel+intents+armor) == combined
    assert apply(apply(body,armor),intents+steel) == combined
    assert apply(apply(body,intents+steel),armor) == combined
    assert apply(combined,armor+intents+steel) == combined
    # Independently produce the expected final records. Check every field,
    # including original DLC buff/use/knowledge actions and armor model edits.
    expected = copy.deepcopy(items)
    affected = {r['key'] for r in intents}
    base_price_ids = {r['key'] for r in intents[:706]}
    dlc_price_ids = affected - base_price_ids
    for row in expected:
        if row['equip_type_info'] in armor_rule['match']['equip_type_info']:
            for prefab in row['prefab_data_list']:
                prefab['tribe_gender_list'] = []
        if row['key'] in base_price_ids:
            for price in row['price_list']:
                price['price']['price'] = 1
        if row['key'] in dlc_price_ids:
            for prices in [row['price_list']] + [e['buy_price_list'] for e in row['enchant_data_list'] or []]:
                for price in prices:
                    if price['key'] == 1:
                        price['price']['price'] = 1
        if row['key'] == 1000594:
            row['enchant_data_list'][0]['equip_buffs'][0]['level'] = 20
    assert parser.parse_table('iteminfo',combined,None) == expected
    prefix = 'files/0008/gamedata/binarystaticinfo__/bin/'
    loc = parser.parse_table('localstringinfo',payload[prefix+'localstringinfo.staticinfobody'],
                             payload[prefix+'localstringinfo.staticinfoheader'])
    dlc_loc = parser.parse_table('localstringinfo',
        (args.dlc_data/'0008/gamedata/binarystaticinfo__/bin/localstringinfo.dlc_01.staticinfobody').read_bytes(),
        (args.dlc_data/'0008/gamedata/binarystaticinfo__/bin/localstringinfo.dlc_01.staticinfoheader').read_bytes())
    dlc_loc_ids = {r['key'] for r in dlc_loc}
    # LocalStringInfo stores these names as hashed keys with empty string_key.
    # Match the appended records to all 15 actual tab translation sets.
    labels = loc[-9:]
    assert len({r['key'] for r in labels}) == 9
    assert not {r['key'] for r in labels} & dlc_loc_ids
    label_ids = [str(r['game_string']['index']) for r in labels]
    translations = read(Path(__file__).with_name('translations.json'))
    checked_languages = []
    for path,data in payload.items():
        if not path.endswith('/localstringinfo.paloc'):
            continue
        lang = path.split('/')[-2]
        added = parser.parse_paloc_bytes(data)[-9:]
        assert [r['string_key'] for r in added] == label_ids
        assert [r['string_value'] for r in added] == translations[lang]
        checked_languages.append(lang)
    assert set(checked_languages) == set(translations)
    report = dict(package_sha256=hashlib.sha256(args.package.read_bytes()).hexdigest(),
        reference_dlc_scope='Current base plus 2.04.05 DLC, offline only',
        base_and_dlc_price_intents=len(intents),equip_all_armor_intents=len(armor),
        steelheart_intents=len(steel),both_orders_and_repeat_application='PASS',
        all_record_fields_match_independent_expected_result=True,
        original_dlc_effects_actions_and_other_currency_prices_preserved=True,
        all_40_reference_dlc_crafting_books_in_books_tab=True,total_book_offers=111,
        all_817_previous_stock_records_byte_identical=True,added_material_offers=24,
        natural_pearl_in_materials=True,
        nine_localized_category_keys_absent_from_reference_dlc_overrides=True,
        game_launch_or_mount_performed=False)
    args.report.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,indent=2))


if __name__ == '__main__':
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument('--expected-version', default='17.4.1')
    for name in ['parser-dir','package','previous-package','item-body','dlc-items','dlc-data','equip-all','steelheart','report']:
        cli.add_argument('--'+name,type=Path,required=True)
    run(cli.parse_args())
