"""Restore Alden's original offers without changing the existing mod stock.

The original item prices remain untouched by this module. The existing global
1 Copper recipe still applies to the nine items shared with the mod catalog.
Only the new offers' stock/save identities are reassigned to avoid collisions.
"""
import copy
import json
import struct
import sys
from pathlib import Path

import lz4.block

from ui_binary import Reader, number, string, strings

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parents[2] / 'tooling'))
from crimson_mod_tools.build_204 import Inputs
from crimson_mod_tools.staticinfo import Table

PREFIX = 'files/0008/gamedata/binarystaticinfo__/bin/'
ORIGINAL_KEY = 580995467  # hashlittle(b'ui_aldencategory_original', 0xc5ede)
ORIGINAL_LOC = (ORIGINAL_KEY << 32) | 0xb0
ORIGINAL_IDS = (
    6001, 1003172, 1003678, 1004663, 1004650, 11001, 950003, 950010,
    950004, 950002, 950001, 1003848, 950006, 950007, 10026, 790002,
    950005, 1000149, 950008, 1001466, 1001551, 1001552, 1001553,
    1001556, 1001563, 1001594, 1001595, 1001508, 1001153, 1002112,
    1002116, 1002117, 1002672, 1002673, 1000878, 700001, 1000659,
    795009, 1001117, 1000853,
)


def add_header(data):
    """Edit only the primitive header subtree; retain opaque native bindings."""
    r = Reader(data)
    needle = strings(['alden-category-headers', 'inherit-no-pickable'])
    assert data.count(needle) == 1
    r.pos = data.index(needle)
    headers = r.node()
    assert headers.id == 'AldenCategoryHeaders' and len(headers.children) == 8
    previous = [n.encode() for n in headers.children]
    original = copy.deepcopy(headers.children[-1])
    for n in original.walk():
        n.names[0] = n.names[0].removesuffix('7') + '8'
        n.css = [s.replace('alden-tab-7', 'alden-tab-8') for s in n.css]
        if n.names[1]:
            assert n.names[1] == 'UI_AldenCategory_Contracts'
            n.names[1] = 'UI_AldenCategory_Original'
        if n.optional[2] is not None:
            assert n.optional[2] == ['50']
            n.optional[2] = ['40']
    headers.children.append(original)
    all_count = [n for n in headers.children[0].walk() if n.id == 'AldenCategoryCount0']
    assert len(all_count) == 1 and all_count[0].optional[2] == ['567']
    all_count[0].optional[2] = ['607']
    assert [n.encode() for n in headers.children[1:8]] == previous[1:]
    rebuilt = data[:headers.start] + headers.encode() + data[headers.end:]
    r = Reader(rebuilt); r.pos = headers.start
    assert len(r.node().children) == 9
    assert rebuilt.count(string('need-trigger-downonce-release') + string('True')) == 2
    return rebuilt


def split_stock(record, count):
    marker = struct.pack('<HQQ', 3101, 1000000, 1000000)
    starts = []
    pos = record.find(marker)
    while pos >= 0:
        starts.append(pos)
        pos = record.find(marker, pos + 1)
    assert len(starts) == count and starts[0] == 81
    assert struct.unpack_from('<I', record, 49)[0] == count
    assert struct.unpack_from('<I', record, 77)[0] == count
    starts.append(len(record) - 10)
    rows = [record[a:b] for a, b in zip(starts, starts[1:])]
    assert all(len(row) in (127, 140) for row in rows)
    return rows


def add_original_stock(payload, vanilla_root, parser, prices):
    sources = Inputs(vanilla_root, 'b26361232')
    vanilla = sources.table('storeinfo', 2)
    body_path, header_path = [PREFIX + 'storeinfo.staticinfo' + s for s in ('body', 'header')]
    current = Table(payload[body_path], payload[header_path], 2)
    old = current.require(3101, 'Store_Her_General').data
    original = vanilla.require(3101, 'Store_Her_General').data
    stock = split_stock(old, 567)
    original_stock = split_stock(original, 40)
    catalog = json.loads((HERE.parents[2] / 'recipes/alden-v17-b26361232/categories.json').read_text(encoding='utf-8'))
    for i, (row, item) in enumerate(zip(stock, catalog['items'], strict=True)):
        assert struct.unpack_from('<II', row, 26) == (i, i)
        assert struct.unpack_from('<I', row, 43)[0] == item['id']
    appended = []
    for i, row in enumerate(original_stock):
        assert struct.unpack_from('<I', row, 43)[0] == ORIGINAL_IDS[i]
        assert struct.unpack_from('<I', row, 102)[0] == ORIGINAL_IDS[i]
        rebuilt = bytearray(row)
        struct.pack_into('<II', rebuilt, 26, 567 + i, 567 + i)
        assert rebuilt[:26] == row[:26] and rebuilt[34:] == row[34:]
        appended.append(bytes(rebuilt))
    prefix = bytearray(old[:81])
    struct.pack_into('<I', prefix, 49, 607)
    struct.pack_into('<I', prefix, 77, 607)
    assert old[-10:] == original[-10:]
    merged = bytes(prefix) + b''.join(stock + appended) + old[-10:]
    updated = current.replace({3101: merged})
    assert split_stock(merged, 607)[:567] == stock
    # Independent DMM parser validates all lengths and nested stock conditions.
    parsed_before = parser.parse_table('storeinfo', current.body, current.header)
    parsed_vanilla = parser.parse_table('storeinfo', vanilla.body, vanilla.header)
    parsed_after = parser.parse_table('storeinfo', updated.body, updated.header)
    before_alden = next(r for r in parsed_before if r['key'] == 3101)
    vanilla_alden = next(r for r in parsed_vanilla if r['key'] == 3101)
    expected = copy.deepcopy(before_alden)
    expected['buyable_stock_count'] = 607
    for i, row in enumerate(vanilla_alden['stock_data_list']):
        row = copy.deepcopy(row)
        row['raw_d'] = row['raw_e'] = 567 + i
        expected['stock_data_list'].append(row)
    assert next(r for r in parsed_after if r['key'] == 3101) == expected
    assert [r for r in parsed_after if r['key'] != 3101] == [r for r in parsed_before if r['key'] != 3101]
    mod_ids = {i['id'] for i in catalog['items']}
    overlaps = sorted(set(ORIGINAL_IDS) & mod_ids)
    assert len(overlaps) == 9 and len(mod_ids | set(ORIGINAL_IDS)) == 598
    price_ids = {i['key'] for t in prices['targets'] for i in t['intents']}
    assert price_ids == mod_ids and price_ids & set(ORIGINAL_IDS) == set(overlaps)
    payload[body_path], payload[header_path] = updated.body, updated.header
    return dict(mod_offers=567, original_offers=40, total_offers=607, unique_items=598,
                one_copper_original_item_ids=overlaps, unchanged_native_price_items=31,
                existing_mod_rows_byte_identical=True, original_stock_conditions_preserved=True,
                original_stock_indices=[567, 606], original_save_indices=[567, 606],
                other_stores_unchanged=True, independent_dmm_parse='PASS', input_hashes=sources.used)


def add_localization(payload, parser):
    translations = json.loads((HERE / 'translations.json').read_text(encoding='utf-8'))
    assert len(translations) == 15 and all(len(v) == 9 for v in translations.values())
    bp, hp = [PREFIX + 'localstringinfo.staticinfo' + s for s in ('body', 'header')]
    body, header = payload[bp], payload[hp]
    count = struct.unpack_from('<H', header)[0]
    assert len(header) == 2 + count * 8
    assert ORIGINAL_KEY not in [struct.unpack_from('<I', header, 2 + i * 8)[0] for i in range(count)]
    payload[bp] = body + number(0) + number(ORIGINAL_KEY) + b'\0\1\x0b' + struct.pack('<Q', ORIGINAL_LOC) + string(str(ORIGINAL_LOC))
    payload[hp] = struct.pack('<H', count + 1) + header[2:] + struct.pack('<II', ORIGINAL_KEY, len(body))
    languages = []
    for path, source in list(payload.items()):
        if not path.endswith('/localstringinfo.paloc'):
            continue
        lang = path.split('/')[-2]
        assert lang in translations
        assert source[:9] == b'paloc\0\0\0\0' and source[17:512] == bytes(495)
        compressed, size = struct.unpack_from('<II', source, 9)
        assert compressed == len(source) - 512
        raw = lz4.block.decompress(source[512:], uncompressed_size=size)
        entries = parser.parse_paloc_bytes(source)
        assert parser.serialize_paloc(entries) == raw
        assert not any(e['string_key'] == str(ORIGINAL_LOC) for e in entries)
        added = dict(unk_id=11, string_key=str(ORIGINAL_LOC), string_value=translations[lang][8])
        rebuilt = parser.serialize_paloc(entries + [added])
        assert rebuilt[:len(raw) - 4] == raw[:-4]
        block = lz4.block.compress(rebuilt, store_size=False)
        container = bytearray(source[:512])
        struct.pack_into('<II', container, 9, len(block), len(rebuilt))
        container += block
        assert parser.parse_paloc_bytes(bytes(container)) == entries + [added]
        payload[path] = bytes(container)
        languages.append(lang)
    assert set(languages) == set(translations)
    return dict(languages=sorted(languages), existing_translation_records_unchanged=True,
                localization_key=ORIGINAL_LOC, new_key='UI_AldenCategory_Original')
