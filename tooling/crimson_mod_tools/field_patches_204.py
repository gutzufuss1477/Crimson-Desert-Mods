"""Convert the existing Alden/Steelheart edits to DMM field-level intents.

Uses the official dmm-parser supplied by the caller. No parser, game tables
or third-party mod assets are distributed by this module. The conversion is
accepted only when its serialized result exactly matches the legacy patches
on the supported vanilla table. Runtime manifests contain no file offsets.
"""
import struct


def apply_legacy(body, patches):
    result = bytearray(body)
    for patch in patches:
        old, new = bytes.fromhex(patch['original']), bytes.fromhex(patch['patched'])
        offset = patch['offset']
        if len(old) != len(new) or result[offset:offset + len(old)] != old:
            raise ValueError('Legacy patch does not match the supplied vanilla table')
        result[offset:offset + len(old)] = new
    return bytes(result)


def field_manifest(title, version, description, intents):
    return {
        'format': 3, 'format_minor': 1,
        'modinfo': {'title': title, 'author': 'Blablup', 'version': version,
                    'description': description},
        'targets': [{'file': 'iteminfo.pabgb', 'intents': intents}],
    }


def verify_equivalent(parser, body, patches, intents):
    result = parser.apply_intents('iteminfo', body, None, intents)
    if len(result['outcomes']) != len(intents) or any(
            outcome['status'] != 'applied' for outcome in result['outcomes']):
        raise ValueError('Field-level conversion did not apply completely')
    if result['body'] != apply_legacy(body, patches):
        raise ValueError('Field-level conversion differs from the released effect')


def alden_prices(parser, body, legacy, catalog_keys, version='17.2.1'):
    items = parser.parse_iteminfo_from_bytes(body)
    if parser.serialize_iteminfo(items) != body:
        raise ValueError('The parser does not round-trip this game build exactly')
    by_name = {item['string_key']: item for item in items}
    if len(by_name) != len(items):
        raise ValueError('Ambiguous item names')
    intents, used = [], set()
    for patch in legacy['patches']:
        name, suffix = patch['description'].rsplit(':', 1)
        if suffix.strip() != 'purchase price -> 1' or name not in by_name:
            raise ValueError('Unexpected price recipe')
        item = by_name[name]
        original = bytes.fromhex(patch['original'])
        if len(original) != 20:
            raise ValueError('Unexpected price record size')
        key, price, sym_no, wrapper = struct.unpack('<IQII', original)
        expected = {'key': key, 'price': {'price': price, 'sym_no': sym_no,
                                         'item_info_wrapper': wrapper}}
        indices = [i for i, value in enumerate(item['price_list']) if value == expected]
        if len(indices) != 1 or bytes.fromhex(patch['patched']) != struct.pack('<IQII', key, 1, sym_no, wrapper):
            raise ValueError('Ambiguous or non-price patch')
        field = f'price_list[{indices[0]}].price.price'
        identity = item['key'], field
        if identity in used:
            raise ValueError('Duplicate price target')
        used.add(identity)
        intents.append({'entry': name, 'key': item['key'], 'field': field,
                        'op': 'set', 'new': 1})
    if len(intents) != 706 or {key for key, _ in used} != set(catalog_keys):
        raise ValueError('Catalog or price-patch scope changed')
    verify_equivalent(parser, body, legacy['patches'], intents)
    return field_manifest('Alden Shop - 1 Copper', version,
        '567 items at 1 Copper. Field-level prices compose with Equip All Armor; '
        'use with the matching Alden Shop package and category ASI.', intents)


def steelheart_stamina(parser, body, legacy, version='10'):
    items = parser.parse_iteminfo_from_bytes(body)
    item = next(item for item in items if item['key'] == 1000594)
    if item['string_key'] != 'HorseShoe_HorseArmor_Shoe_III':
        raise ValueError('Steelheart item identity changed')
    matches = [(i, j) for i, enchant in enumerate(item['enchant_data_list'] or [])
               for j, buff in enumerate(enchant['equip_buffs'])
               if buff == {'buff': 1000046, 'level': 4}]
    if len(matches) != 1:
        raise ValueError('Steelheart stamina regeneration buff is ambiguous')
    i, j = matches[0]
    intents = [{'entry': item['string_key'], 'key': item['key'],
                'field': f'enchant_data_list[{i}].equip_buffs[{j}].level',
                'op': 'set', 'new': 20}]
    verify_equivalent(parser, body, legacy['patches'], intents)
    return field_manifest('Steelheart Horseshoes - 20 Stamina Regen', version,
        'Sets the Steelheart Horseshoes stamina-regeneration equip-buff level '
        'from 4 to 20. Field-level edit composes with Equip All Armor.', intents)
