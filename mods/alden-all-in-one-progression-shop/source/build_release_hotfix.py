"""Rebase a published Alden package without changing its catalog or behavior.

Builds 17.3.3 (regular) or 17.4.2 (DLC) for Steam 26480954. The new Alfred
merchant feature is deliberately not an input. Never mounts or publishes.
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

import capstone
import lz4.block
import build_runtime_shop as runtime
from original_shop import Table, PREFIX

HERE=Path(__file__).resolve().parent
PROFILE=HERE.parents[2]/'recipes/alden-hotfix-b26480954/native-profile.json'
VARIANTS={
    'regular': dict(version='17.3.3',previous='17.3.2',zip='Alden_Shop.zip',offers=607,intents=706,
        sha='8b90164dc3beefacc10489686d8145cdf35e07d29e2479932b9c60ecd799f1ba',readme='PACKAGE_README.txt'),
    'dlc': dict(version='17.4.2',previous='17.4.1',zip='Alden_Shop_DLC.zip',offers=841,intents=958,
        sha='7b8d36f42c9aab54dd8eae75fc81d380d2c3608a906ca3575a79204907b0d521',readme='DLC_PACKAGE_README.txt'),
}


def read(path): return json.loads(path.read_text(encoding='utf-8-sig'))


def rebase_data(payload,current,parser):
    bp,hp=[PREFIX+'storeinfo.staticinfo'+s for s in ('body','header')]
    old=Table(payload[bp],payload[hp],2)
    native=Table(*[(current/n.removeprefix('files/')).read_bytes() for n in (bp,hp)],2)
    alden=old.require(3101,'Store_Her_General').data
    updated=native.replace({3101:alden})
    parsed=parser.parse_table('storeinfo',updated.body,updated.header)
    assert parser.serialize_table('storeinfo',parsed)==updated.body
    assert next(r for r in parsed if r['key']==3101)==next(r for r in parser.parse_table('storeinfo',old.body,old.header) if r['key']==3101)
    payload[bp],payload[hp]=updated.body,updated.header
    bp,hp=[PREFIX+'localstringinfo.staticinfo'+s for s in ('body','header')]
    body,header=[(current/n.removeprefix('files/')).read_bytes() for n in (bp,hp)]
    count=struct.unpack_from('<H',header)[0]
    old_count=struct.unpack_from('<H',payload[hp])[0]
    old_index=[struct.unpack_from('<II',payload[hp],2+i*8) for i in range(old_count)]
    native_keys={struct.unpack_from('<I',header,2+i*8)[0] for i in range(count)}
    appended=old_index[-9:]
    assert not native_keys.intersection(k for k,_ in appended)
    outbody=bytearray(body);outheader=bytearray(struct.pack('<H',count+9)+header[2:])
    for i,(key,start) in enumerate(appended):
        end=appended[i+1][1] if i<8 else len(payload[bp])
        outheader+=struct.pack('<II',key,len(outbody));outbody+=payload[bp][start:end]
    before=parser.parse_table('localstringinfo',body,header)
    labels=parser.parse_table('localstringinfo',payload[bp],payload[hp])[-9:]
    assert parser.parse_table('localstringinfo',bytes(outbody),bytes(outheader))==before+labels
    payload[bp],payload[hp]=bytes(outbody),bytes(outheader)
    languages=[]
    for path,old in list(payload.items()):
        if not path.endswith('/localstringinfo.paloc'): continue
        source=(current/path.removeprefix('files/')).read_bytes()
        entries=parser.parse_paloc_bytes(source)
        custom=parser.parse_paloc_bytes(old)[-9:]
        assert [r['string_key'] for r in custom]==[str(r['game_string']['index']) for r in labels]
        assert not {r['string_key'] for r in entries}.intersection(r['string_key'] for r in custom)
        raw=parser.serialize_paloc(entries+custom)
        compressed=lz4.block.compress(raw,store_size=False)
        result=bytearray(source[:512]);struct.pack_into('<II',result,9,len(compressed),len(raw));result+=compressed
        assert parser.parse_paloc_bytes(bytes(result))==entries+custom
        payload[path]=bytes(result);languages.append(path.split('/')[-2])
    assert len(languages)==15
    return dict(alden_store_bytes_unchanged=True,other_current_merchants_preserved=len(native.records)-1,
        alfred_unchanged=True,current_localization_preserved=True,tab_translations_unchanged=True,languages=languages)


def build_native(args,variant,profile):
    native=args.output/'native-build'
    shutil.copytree(args.previous_build/'native-build',native,dirs_exist_ok=True,ignore=shutil.ignore_patterns('out','*.log'))
    addresses={k:int(v,16) for k,v in profile['addresses'].items()}
    exe=args.game_exe.read_bytes()
    assert runtime.sha(exe)==profile['exe_sha256']
    def raw(name,size=16): return runtime.exe_at(exe,addresses[name],size)
    def guard(name): return '    {%s, {%s}},'%(name,','.join('0x%02x'%b for b in raw(name)))
    p=(native/'profile_generated.hpp').read_text()
    p=re.sub(r'ExeHash\[\]="[a-f0-9]+"','ExeHash[]="'+profile['exe_sha256']+'"',p)
    p=re.sub(r'(inline constexpr std::uintptr_t (\w+) = )0x[0-9a-f]+;',lambda m:m[1]+hex(addresses[m[2]])+';',p)
    names=['Init','Destroy','Fill','TabChanged','SetCount','BuildStock','BindTab','StoreInfo','ItemInfo','Show','Hide','Attribute']
    p=re.sub(r'inline constexpr Guard Guards\[\] = \{.*?\};','inline constexpr Guard Guards[] = {\n'+'\n'.join(guard(n) for n in names)+'\n};',p,flags=re.S)
    (native/'profile_generated.hpp').write_text(p)
    p=(native/'runtime_generated.hpp').read_text()
    p=re.sub(r'(inline constexpr std::uintptr_t (\w+)=)0x[0-9a-f]+;',lambda m:m[1]+hex(addresses[m[2]])+';',p)
    p=re.sub(r'Version\[\]="[^"]+"',f'Version[]="Alden Category Tabs {variant["version"]} / Steam 26480954 / runtime UI augmentation"',p)
    p=re.sub(r'inline constexpr unsigned char NativeMemoryReadCode\[\] = \{.*?\};',runtime.array('NativeMemoryReadCode',raw('MemoryRead',0x40)),p,flags=re.S)
    names=['Deserialize','MemoryRead','LoadCss','CompileCss','DecodeCss','LookupCss','SetString','ReleaseString','ReleaseBuffer']
    p=re.sub(r'inline constexpr Guard Guards\[\] = \{.*?\};','inline constexpr Guard Guards[] = {\n'+'\n'.join(guard(n) for n in names)+'\n};',p,flags=re.S)
    (native/'runtime_generated.hpp').write_text(p)
    # Preserve all runtime UI edits and input/output hashes, including BI 3.3.
    vanilla=(args.current_data/'0012/ui/bin__/npcstorebuyview2_game.uibinary').read_bytes()
    assert vanilla==(native/'fixtures/vanilla.bin').read_bytes()
    address=addresses['StyleConsumer'];consumer=raw('StyleConsumer',0x264)
    decoder=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);decoder.detail=True
    calls=[];references=[]
    for ins in decoder.disasm(consumer,0x140000000+address):
        if ins.mnemonic=='call':
            assert ins.bytes[0]==0xe8 and ins.size==5
            calls.append((ins.address-(0x140000000+address)+1,ins.operands[0].imm-0x140000000))
        for op in ins.operands:
            if op.type==capstone.CS_OP_MEM and op.mem.base==capstone.x86.X86_REG_RIP:
                references.append((ins.address-(0x140000000+address)+ins.disp_offset,ins.address+ins.size+op.mem.disp-0x140000000))
    assert {v for _,v in calls}=={addresses[n] for n in ['LoadCss','FinalizeStyles','BindStyles','ConsumerHelper1','ConsumerHelper2','ConsumerHelper3','ConsumerHelper4','ConsumerHelper5','ReleaseString']}
    assert len(references)==2
    code=['#pragma once',runtime.array('NativeStyleConsumerCode',consumer),'struct ConsumerFixup {unsigned offset; std::uintptr_t target;};']
    code += [f'inline constexpr std::uintptr_t Consumer{n}={addresses[n]:#x};' for n in ['StyleConsumer','LoadCss','FinalizeStyles','BindStyles']]
    code += ['inline constexpr const char* OriginalStyleNames[] = {'+', '.join(json.dumps(s) for s in runtime.stylesheet_list(vanilla)[2])+'};']
    for name,rows in [('ConsumerCalls',calls),('ConsumerReferences',references)]:
        code.append('inline constexpr ConsumerFixup '+name+'[] = {'+','.join('{%d,0x%x}'%r for r in rows)+'};')
    (native/'consumer_test_generated.hpp').write_text('\n'.join(code)+'\n')
    version=variant['version']
    (native/'version_generated.h').write_text('#pragma once\n#define ALDEN_FILE_VERSION '+version.replace('.',',')+',0\n#define ALDEN_FILE_FLAGS 0\n#define ALDEN_VERSION_TEXT "'+version+'\\0"\n')
    for name in ['native_tabs.cpp','filter_core.hpp','catalog_generated.hpp','runtime_ui.hpp','ui_patch_core.hpp','native_tests.cpp','runtime_ui_tests.cpp']:
        assert (native/name).read_bytes()==(args.previous_build/'native-build'/name).read_bytes()
    subprocess.run(['cmd.exe','/d','/c','build_runtime.cmd'],cwd=native,check=True)
    asi=native/'out/Alden Category Tabs.asi'
    start=time.time_ns();ctypes.WinDLL(str(asi.resolve()))
    log=Path(str(asi)+'.log')
    for _ in range(40):
        if log.exists() and log.stat().st_mtime_ns>=start and 'REFUSED: wrong process' in log.read_text(errors='replace'): break
        time.sleep(.1)
    else: raise AssertionError('Wrong-process guard failed')
    return asi.read_bytes()


def build(args):
    variant=VARIANTS[args.variant];profile=read(PROFILE)
    args.output.mkdir(parents=True,exist_ok=True)
    previous=args.previous_build/variant['zip']
    assert runtime.sha(previous.read_bytes())==variant['sha']
    with zipfile.ZipFile(previous) as z: payload={n:z.read(n) for n in z.namelist()}
    previous_game={n:v for n,v in payload.items() if n.startswith('files/')}
    patch=json.loads(payload['Preise.json']);targets=copy.deepcopy(patch['targets'])
    assert sum(len(t['intents']) for t in targets)==variant['intents']
    sys.path.insert(0,str(args.parser_dir.resolve()))
    import dmm_parser as parser
    report=rebase_data(payload,args.current_data,parser)
    payload['Alden Category Tabs.asi']=build_native(args,variant,profile)
    patch['modinfo']['version']=variant['version']
    if args.variant=='dlc':
        patch['modinfo']['description']='801 mod offers at 1 Copper. Includes all 234 existing DLC offers, Natural Pearl and 40 DLC crafting books at Alden. Requires Charting the Unknown. Use matching 17.4.2 package and ASI.'
    assert patch['targets']==targets
    payload['Preise.json']=json.dumps(patch,ensure_ascii=False,indent=2).encode()
    manifest=json.loads(payload['manifest.json']);manifest['version']=variant['version']
    manifest['description']=f'Steam 26480954 compatibility hotfix. {variant["offers"]} existing offers, nine localized categories. Update both package and ASI. Catalog, prices and stocks unchanged. Offline validated; this build awaits gameplay confirmation.'
    payload['manifest.json']=json.dumps(manifest,ensure_ascii=False,indent=2).encode()
    payload['README.txt']=runtime.crlf((HERE/variant['readme']).read_text(encoding='utf-8'))
    assert runtime.sha(args.better_inventory.read_bytes())==runtime.BETTER_SHA256
    with zipfile.ZipFile(args.better_inventory) as z:
        better={p.partition('/files/')[2].partition('/')[2].lower() for p in z.namelist() if '/files/' in p}
        assert z.read('Better_Inventory_and_Trade_UI/'+runtime.BINARY)==(args.output/'native-build/fixtures/better.bin').read_bytes()
    own={p.split('/',2)[2].lower() for p in previous_game}
    assert len(own)==20 and not own&better
    assert not any('.dlc_01.' in p or 'iteminfo' in p or p.endswith('.dds') for p in own)
    assert set(previous_game)=={n for n in payload if n.startswith('files/')}
    destination=args.output/variant['zip']
    with zipfile.ZipFile(destination,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for n,v in sorted(payload.items()):
            info=zipfile.ZipInfo(n,(2026,11,6,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED
            z.writestr(info,v,compress_type=zipfile.ZIP_DEFLATED,compresslevel=9)
    with zipfile.ZipFile(destination) as z: assert z.testzip() is None and all(z.read(n)==v for n,v in payload.items())
    for name in ['Alden Category Tabs.asi','README.txt']: (args.output/name).write_bytes(payload[name])
    report.update(version=variant['version'],variant=args.variant,steam_build='26480954',exe_sha256=profile['exe_sha256'],
        package_sha256=runtime.sha(destination.read_bytes()),asi_sha256=runtime.sha(payload['Alden Category Tabs.asi']),
        catalog_offers=variant['offers'],price_intents=variant['intents'],price_targets_unchanged=True,
        runtime_logic_catalog_and_ui_edits_unchanged=True,shared_better_inventory_paths=[],
        changed_game_files=[n for n,v in previous_game.items() if payload[n]!=v],
        native_regressions='PASS',runtime_ui_regressions='PASS',wrong_process_guard='PASS',
        alfred_feature_included=False,in_game_tested=False,dlc_runtime_tested=False,
        reported_dlc_crash_reproduced=False,installed=False,published=False)
    (args.output/'VALIDATION.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))


if __name__=='__main__':
    cli=argparse.ArgumentParser(description=__doc__)
    cli.add_argument('--variant',choices=list(VARIANTS),required=True)
    for name in ['previous-build','current-data','parser-dir','game-exe','better-inventory','output']:
        cli.add_argument('--'+name,type=Path,required=True)
    build(cli.parse_args())
