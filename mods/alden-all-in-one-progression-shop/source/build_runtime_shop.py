"""Build Alden's ASI UI augmentation. No shared UI replacement is packaged.

Required local inputs: published Alden 17.1, original game buy UI, original
Better Inventory 3.3, and the supported executable. Game-derived generated
headers and complete game/UI assets stay in the caller's local output folder.
"""
import argparse
import ctypes
import hashlib
import json
import shutil
import struct
import subprocess
import time
import zipfile
import capstone
from pathlib import Path

from runtime_build_inputs import (ALDEN_SHA256, BETTER_SHA256, BINARY, HTML,
                                 CSS as SHARED_CSS, EXPECTED_OUTPUT_HASHES,
                                 read_archive, unique, stylesheet_list, crlf)
from ui_binary import Reader, number, string, strings

HERE = Path(__file__).resolve().parent
VERSION = '17.2.2'
HELPER_VERSION = VERSION
EXE_SHA = 'df531a585bf84ceb47779304cc8b3caaf976aea88424c32605885623cd4519c7'
VANILLA_SHA = '68ab1eb625358434a8d65912d5a904085bcafb620c8d4ca88087d12dc8de396b'
CSS = 'files/0012/ui/xml/gamemain/play/aldencategorytabs.css'
STYLE = 'UI/GameMain/Play/AldenCategoryTabs.css'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def make_edits(alden_binary, vanilla):
    bar_at = unique(alden_binary, strings(sorted(['!cpp-none', 'alden-category-bar', 'inherit-pickable'])))
    list_at = unique(alden_binary, strings(['two-column-list-section-wrap'])) - 1
    bar = alden_binary[bar_at:list_at]
    assert bar.count(string('need-trigger-downonce-release') + string('True')) == 2
    container = unique(vanilla, strings(['npc-store-container']))
    child_count = container + len(strings(['npc-store-container'])) + 1
    before_list = unique(vanilla, strings(['two-column-list-section-wrap'])) - 1
    attrs = unique(vanilla, string('sequencer-gameplay-relfect-event'))
    style_start, style_end, paths = stylesheet_list(vanilla)
    assert vanilla[child_count - 1:child_count + 4] == b'\1' + number(5)
    assert vanilla[attrs - 5:attrs] == b'\1' + number(39)
    assert vanilla[before_list] == 1 and len(paths) == 46
    edits = sorted([
        (child_count, number(5), number(6)),
        (before_list, b'', b'\1' + bar),
        (attrs - 4, number(39), number(40)),
        (attrs, b'', string('selector-itemlist-tabbar') + string('#AldenCategoryTabs')),
    ])
    return edits


def apply_edits(data, edits):
    output = bytearray()
    cursor = 0
    inverse = []
    for offset, before, after in edits:
        if offset < cursor or data[offset:offset + len(before)] != before:
            raise ValueError('UI edit preimage mismatch')
        output += data[cursor:offset]
        inverse.append((len(output), after, before))
        output += after
        cursor = offset + len(before)
    output += data[cursor:]
    restored = bytes(output)
    for offset, after, before in reversed(inverse):
        if restored[offset:offset + len(after)] != after:
            raise ValueError('UI edit postimage mismatch')
        restored = restored[:offset] + before + restored[offset + len(after):]
    if restored != data:
        raise ValueError('Unrelated UI bytes changed')
    return bytes(output)


def array(name, data):
    assert data
    lines = [f'inline constexpr unsigned char {name}[] = {{']
    for start in range(0, len(data), 24):
        lines.append('    ' + ','.join(f'0x{x:02x}' for x in data[start:start + 24]) + ',')
    return '\n'.join(lines + ['};'])


def exe_at(executable, rva, size):
    pe = struct.unpack_from('<I', executable, 0x3c)[0]
    count = struct.unpack_from('<H', executable, pe + 6)[0]
    optional = struct.unpack_from('<H', executable, pe + 20)[0]
    for n in range(count):
        start = pe + 24 + optional + n * 40
        _, va, raw_size, raw_start = struct.unpack_from('<4I', executable, start + 8)
        if va <= rva and rva + size <= va + raw_size:
            return executable[raw_start + rva - va:raw_start + rva - va + size]
    raise ValueError('RVA outside raw sections')


def build(args):
    alden = read_archive(args.alden, ALDEN_SHA256)
    third = read_archive(args.better_inventory, BETTER_SHA256)
    prefix = 'Better_Inventory_and_Trade_UI/'
    better = {n.removeprefix(prefix): v for n, v in third.items() if n.startswith(prefix + 'files/')}
    vanilla = args.vanilla_ui.read_bytes()
    exe = args.exe.read_bytes()
    if sha(vanilla) != VANILLA_SHA or sha(exe) != EXE_SHA:
        raise ValueError('Unsupported original UI or executable')
    edits = make_edits(alden[BINARY], vanilla)
    outputs = [apply_edits(d, edits) for d in [vanilla, better[BINARY]]]
    # Use the exact previously working trees, retaining all original stylesheet
    # dependencies. Category CSS is bound later by the native stylesheet hook.
    if outputs[0] != alden[BINARY] or [sha(data) for data in outputs] != EXPECTED_OUTPUT_HASHES:
        raise ValueError('Runtime augmentation differs from known category trees')
    assert all(stylesheet_list(before)[2] == stylesheet_list(after)[2]
               for before,after in zip([vanilla,better[BINARY]],outputs))
    for old, new in zip([vanilla, better[BINARY]], outputs):
        subtrees = []
        for data in [old, new]:
            r = Reader(data); r.pos = data.index(strings(['two-column-list-section-wrap']))
            subtrees.append(r.node().encode())
        if subtrees[0] != subtrees[1]:
            raise ValueError('Native buy grid was modified')
    css = b'\xef\xbb\xbf' + crlf((HERE / 'aldencategorytabs.css').read_text(encoding='utf-8'))
    payload = {n: v for n, v in alden.items() if n.startswith('files/') and n not in [BINARY, HTML, SHARED_CSS]}
    payload[CSS] = css
    assert len(payload) == 20 and not set(payload) & set(better)
    assert all(v == alden[n] for n, v in payload.items() if n != CSS)

    out = args.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    native = out / 'native-build'; native.mkdir(exist_ok=True)
    for file in (HERE / 'native').iterdir():
        if file.is_file() and file.suffix in {'.cpp', '.hpp', '.cmd', '.rc'}:
            (native / file.name).write_bytes(file.read_bytes())
    version_parts = [int(part) for part in HELPER_VERSION.split('-')[0].split('.')]
    version_parts += [0] * (4 - len(version_parts))
    (native / 'version_generated.h').write_text(
        '#pragma once\n#define ALDEN_FILE_VERSION ' + ','.join(map(str, version_parts)) +
        '\n#define ALDEN_FILE_FLAGS 0\n#define ALDEN_VERSION_TEXT "' + HELPER_VERSION + '\\0"\n')
    vendor = HERE.parents[1] / 'mining-helmet-always-on/source/src/vendor/minhook'
    # This existing repository dependency carries its complete license/provenance.
    shutil.copytree(vendor, native / 'vendor/minhook', dirs_exist_ok=True)
    fixtures = native / 'fixtures'; fixtures.mkdir(exist_ok=True)
    for name, data in [('vanilla', vanilla), ('better', better[BINARY]), ('vanilla_expected', outputs[0]), ('better_expected', outputs[1])]:
        (fixtures / (name + '.bin')).write_bytes(data)
    footer = string('UI/GameMain/Play/NpcStoreBuyView2.html') + bytes(5)
    assert vanilla.endswith(footer) and better[BINARY].endswith(footer)
    code = ['#pragma once', '#include <span>', '#include <cstddef>', '#include <cstdint>', 'namespace alden::runtime {',
            f'inline constexpr char Version[]="Alden Category Tabs {HELPER_VERSION} / Steam 26361232 / runtime UI augmentation";',
            f'inline constexpr char CssHash[]="{sha(css)}";',
            f'inline constexpr char VanillaInputHash[]="{sha(vanilla)}";',
            f'inline constexpr char BetterInputHash[]="{sha(better[BINARY])}";',
            f'inline constexpr char VanillaOutputHash[]="{sha(outputs[0])}";',
            f'inline constexpr char BetterOutputHash[]="{sha(outputs[1])}";',
            'inline constexpr std::uintptr_t Deserialize=0x41e6d90;',
            'inline constexpr std::uintptr_t MemoryReaderVtable=0x5ab96a8;',
            'inline constexpr std::uintptr_t MemoryRead=0x1601230;',
            'inline constexpr std::uintptr_t LoadCss=0x41bbf30;',
            'inline constexpr std::uintptr_t CompileCss=0x41ac150;',
            'inline constexpr std::uintptr_t DecodeCss=0x41a7e60;',
            'inline constexpr std::uintptr_t LookupCss=0x41a7360;',
            'inline constexpr std::uintptr_t SetString=0x4455f0;',
            'inline constexpr std::uintptr_t ReleaseString=0x3e56e0;',
            'inline constexpr std::uintptr_t ReleaseBuffer=0x1554b50;',
            'inline constexpr std::uintptr_t EmptyString=0x6e9cd20;',
            f'inline constexpr char CssPath[]="{STYLE}";',
            array('CssSource',css),
            array('BuySourceTail', footer), array('NativeMemoryReadCode', exe_at(exe, 0x1601230, 0x40)),
            'struct Guard {std::uintptr_t rva; unsigned char bytes[16];};',
            'inline constexpr Guard Guards[] = {']
    for address in [0x41e6d90,0x1601230,0x41bbf30,0x41ac150,0x41a7e60,0x41a7360,0x4455f0,0x3e56e0,0x1554b50]:
        code.append('    {0x%x, {%s}},' % (address, ','.join('0x%02x' % b for b in exe_at(exe, address, 16))))
    code.append('};')
    for i, (_, before, after) in enumerate(edits):
        if before: code.append(array('Before' + str(i), before))
        code.append(array('After' + str(i), after))
    code += ['struct Edit {std::size_t offset; std::span<const unsigned char> before,after;};', 'inline constexpr Edit Edits[] = {']
    for i, (offset, before, _) in enumerate(edits):
        code.append('    {%d, %s, After%d},' % (offset, 'Before' + str(i) if before else '{}', i))
    code += ['};', f'inline constexpr std::size_t AddedSize={len(outputs[0])-len(vanilla)};', '}']
    # Isolated regression executes the real native consumer's dependency loop,
    # replacing only its external calls with test fixtures. Internal branches
    # and the sequence of CSS loading before UI binding remain actual game code.
    consumer=exe_at(exe,0x38ac3c0,0x264)
    decoder=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64); decoder.detail=True
    calls=[]; references=[]
    for ins in decoder.disasm(consumer,0x140000000+0x38ac3c0):
        if ins.mnemonic=='call':
            assert ins.bytes[0]==0xe8 and ins.size==5
            calls.append((ins.address-(0x140000000+0x38ac3c0)+1,ins.operands[0].imm-0x140000000))
        for operand in ins.operands:
            if operand.type==capstone.CS_OP_MEM and operand.mem.base==capstone.x86.X86_REG_RIP:
                assert ins.disp_size==4
                references.append((ins.address-(0x140000000+0x38ac3c0)+ins.disp_offset,ins.address+ins.size+ins.disp-0x140000000))
    assert {target for _,target in calls}=={0x41bbf30,0x380ebc0,0x41e3b10,0x41e3db0,0x41e3e50,0x4026a10,0x4a8c90,0x5eeca0,0x3e56e0}
    assert len(references)==2
    test_code=['#pragma once',array('NativeStyleConsumerCode',consumer),'struct ConsumerFixup {unsigned offset; std::uintptr_t target;};']
    test_code.append('inline constexpr const char* OriginalStyleNames[] = {'+', '.join(json.dumps(s) for s in stylesheet_list(vanilla)[2])+'};')
    for name,records in [('ConsumerCalls',calls),('ConsumerReferences',references)]:
        test_code.append(f'inline constexpr ConsumerFixup {name}[] = {{'+','.join('{%d,0x%x}'%pair for pair in records)+'};')
    (native/'consumer_test_generated.hpp').write_text('\n'.join(test_code)+'\n')
    (native / 'runtime_generated.hpp').write_text('\n'.join(code) + '\n')
    subprocess.run(['cmd.exe', '/d', '/c', 'build_runtime.cmd'], cwd=native, check=True)
    asi_path = native / 'out/Alden Category Tabs.asi'
    start = time.time_ns(); ctypes.WinDLL(str(asi_path))
    log_path = Path(str(asi_path) + '.log')
    for _ in range(40):
        if log_path.exists() and log_path.stat().st_mtime_ns >= start and 'REFUSED: wrong process' in log_path.read_text(errors='replace'): break
        time.sleep(.1)
    else: raise ValueError('Wrong-process guard failed')
    # Keep the validated field-level price targets, including Equip All Armor
    # compatibility; do not reintroduce the legacy fixed-offset price manifest.
    prices = json.loads((HERE.parents[2] / 'recipes/alden-v17.2.1-b26361232/field-patches.json').read_text(encoding='utf-8'))
    assert prices['format'] == 3 and prices['format_minor'] == 1
    assert sum(len(target['intents']) for target in prices['targets']) == 706
    prices['modinfo']['version'] = VERSION
    manifest = dict(format='crimson_browser_mod_v1', id='AldenShop', title='Alden Shop', author='Blablup',
                    version=VERSION, enabled=True, priority=1, files_dir='files',
                    description='567 items with localized runtime categories and field-level 1 Copper prices. Compatible data edits for original Equip All Armor; optional original Better Inventory 3.3.')
    payload.update({'Alden Category Tabs.asi': asi_path.read_bytes(),
                    'Preise.json': json.dumps(prices, ensure_ascii=False, indent=2).encode(),
                    'manifest.json': json.dumps(manifest, ensure_ascii=False, indent=2).encode(),
                    'LICENSE.txt': alden['NATIVE_LICENSE.txt'],
                    'MinHook-LICENSE.txt': (vendor / 'LICENSE.txt').read_bytes(),
                    'README.txt': crlf((HERE / 'PACKAGE_README.txt').read_text(encoding='utf-8'))})
    dest = out / 'Alden_Shop.zip'
    with zipfile.ZipFile(dest, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for name, data in sorted(payload.items()):
            info = zipfile.ZipInfo(name, (2026, 11, 4, 0, 0, 0)); info.compress_type = zipfile.ZIP_DEFLATED
            z.writestr(info, data)
    with zipfile.ZipFile(dest) as z:
        assert z.testzip() is None and all(z.read(n) == v for n, v in payload.items())
    report = dict(version=VERSION, package_path=str(dest), package_sha256=sha(dest.read_bytes()),
                  asi_sha256=sha(asi_path.read_bytes()), game_payload_files=20, unchanged_data_and_language_files=19,
                  shared_paths_with_original_better_inventory=[], native_buy_grid_preserved=True,
                  order_independent_file_overlay=True, input_hashes=[sha(vanilla), sha(better[BINARY])],
                  output_hashes=[sha(v) for v in outputs], native_callbacks_and_runtime_tests='PASS',
                  wrong_process_guard='PASS', game_process_injected=False, installed=False,
                  in_game_tested=False, dlc_data_validated=False, published=False)
    (out / 'BUILD.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    for arg in ['alden', 'better-inventory', 'vanilla-ui', 'exe', 'output']:
        p.add_argument('--' + arg, type=Path, required=True)
    build(p.parse_args())
