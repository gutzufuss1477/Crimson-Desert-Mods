"""Integration test for DMM field patches using caller-supplied game inputs.

Usage: python test_field_patches_204.py --parser-python DIR --body FILE
       --alden-prices FILE --catalog FILE --steelheart FILE --equip-all FILE
No game or third-party mod data is included. Use the official dmm-parser
binding for the supplied game build (cb1296e for Steam 26361232).
"""
import argparse
from copy import deepcopy
import json
from pathlib import Path
import sys

from crimson_mod_tools.field_patches_204 import alden_prices, steelheart_stamina


def check(parser, body, legacy_alden, catalog, legacy_steelheart, equip_all):
    items = parser.parse_iteminfo_from_bytes(body)
    assert len(items) == 7064 and parser.serialize_iteminfo(items) == body
    keys = {row['id'] for row in catalog['items']}
    alden = alden_prices(parser, body, legacy_alden, keys)['targets'][0]['intents']
    steel = steelheart_stamina(parser, body, legacy_steelheart)['targets'][0]['intents']
    rule, = equip_all['targets'][0]['intents']
    assert rule['field'] == 'prefab_data_list[*].tribe_gender_list'
    assert rule['new'] == [] and rule['op'] == 'set'
    assert set(rule['match']) == {'equip_type_info'}
    # DMM expands this original match/wildcard before applying concrete fields.
    armor = [{'key': item['key'], 'field': f'prefab_data_list[{i}].tribe_gender_list',
              'op': 'set', 'new': []}
             for item in items if item['equip_type_info'] in rule['match']['equip_type_info']
             for i in range(len(item['prefab_data_list']))]
    assert len(armor) == 5148

    def apply(source, intents):
        result = parser.apply_intents('iteminfo', source, None, intents)
        assert len(result['outcomes']) == len(intents)
        assert all(v['status'] == 'applied' for v in result['outcomes'])
        return result['body']

    armor_only = apply(body, armor)
    result = apply(body, armor + alden + steel)
    assert result == apply(body, steel + alden + armor)
    assert result == apply(apply(body, alden + steel), armor)
    assert result == apply(armor_only, alden + steel)
    assert result == apply(result, alden + steel)
    assert result == apply(result, armor)
    assert len(armor_only) == len(result) == 5763906
    expected = parser.parse_iteminfo_from_bytes(armor_only)
    for item in expected:
        if item['key'] in keys:
            for price in item['price_list']:
                price['price']['price'] = 1
        if item['key'] == 1000594:
            item['enchant_data_list'][0]['equip_buffs'][0]['level'] = 20
    assert parser.parse_iteminfo_from_bytes(result) == expected

    # Conversion must not silently broaden its scope or accept a changed base.
    broken = deepcopy(legacy_alden)
    broken['patches'][0]['offset'] += 1
    try:
        alden_prices(parser, body, broken, keys)
    except ValueError:
        pass
    else:
        raise AssertionError('Changed legacy preimage was accepted')
    print('PASS: exact original effects; all 706 prices and Steelheart; both orders; '
          'idempotence; original armor edits preserved; invalid preimage refused.')


if __name__ == '__main__':
    cli = argparse.ArgumentParser(description=__doc__)
    for name in ['parser-python', 'body', 'alden-prices', 'catalog', 'steelheart', 'equip-all']:
        cli.add_argument('--' + name, type=Path, required=True)
    args = cli.parse_args()
    sys.path.insert(0, str(args.parser_python.resolve()))
    import dmm_parser
    read = lambda p: json.loads(p.read_text(encoding='utf-8-sig'))
    check(dmm_parser, args.body.read_bytes(), read(args.alden_prices),
          read(args.catalog), read(args.steelheart), read(args.equip_all))
