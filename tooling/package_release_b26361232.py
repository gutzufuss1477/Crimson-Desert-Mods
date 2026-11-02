"""Promote the approved Bank, AllStats and Healthbar candidates without rebuilding payloads."""
from pathlib import Path
import argparse
import hashlib
import json
import zipfile
from package_candidates_b26361232 import pack

ROOT = Path(__file__).resolve().parents[1]
COMPATIBILITY = (
    'The ASI locates validated code structures instead of depending on fixed addresses or one executable hash. '
    'It may continue working after updates that only move code. Changed code, layouts or ambiguous matches '
    'are refused before hooks are installed. Future compatibility is not guaranteed.'
)
INSTALL_ASI = (
    'Close the game. Remove the previous copy of this mod, including old manual ASIs. '
    'Import this complete ZIP in DMM under ASI Mods, enable it and mount. The INI is included where needed. '
    'Keep your existing settings when upgrading. Keep DMMRuntime.asi and the existing ASI loader. '
    'Do not also install another manual copy.\n'
    'Manual alternative: copy the ASI and required INI beside CrimsonDesert.exe in bin64 with an x64 ASI loader. '
    'DMM is optional for Bank and Mount AllStats; no loader DLL is bundled.\n'
)

def sha(data):
    return hashlib.sha256(data).hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidates', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    approved = json.loads((ROOT / 'recipes/b26361232/release-approval.json').read_text())
    candidates = json.loads((args.candidates / 'VALIDATION.json').read_text())
    assert approved['gameplay_confirmed'] == ['bank', 'allstats', 'healthbar_vanilla_multitarget']
    for folder in ('nexus', 'github'):
        (args.output / folder).mkdir(parents=True, exist_ok=True)
    records = []
    helper = None
    for candidate in candidates['files']:
        kind = candidate['kind']
        if kind not in ('bank', 'allstats', 'healthbar'):
            continue
        path = args.candidates / candidate['file']
        assert sha(path.read_bytes()) == candidate['sha256']
        with zipfile.ZipFile(path) as z:
            assert z.testzip() is None
            original = {n: z.read(n) for n in z.namelist()}
        files = dict(original)
        if kind == 'bank':
            filename, version = 'Bank_Refresh_v1.3.0_b26361232.zip', '1.3.0'
            title = 'Faster Bank Refresh - Gold and Bonds'
            note = ('Gold and active bond timers were confirmed working in game on Steam build 26361232, '
                    '2 November 2026. Defaults: 15 in-game minutes each. Investments/reinvestment stay manual; '
                    'native payouts and risk are unchanged. Restart after INI changes. Shortened saved deadlines '
                    'are not restored by uninstalling. Startup status: %LOCALAPPDATA%\\CrimsonDesertMods\\Bank_Refresh.status.txt.')
            nexus_id = 3669
        elif kind == 'allstats':
            filename, version = 'AllMountsLvL5AllStats_v13_b26361232.zip', '13'
            title = 'All Mounts Level 5 - All Stats'
            note = ('Confirmed working in game on Steam build 26361232, 2 November 2026. '
                    'Changes speed, acceleration, turning and jump. Use AllStats OR Speed Only, never both. '
                    'Steelheart Horseshoes is optional and separate. Startup status: %LOCALAPPDATA%\\CrimsonDesertMods\\Mount_AllStats.status.txt.')
            nexus_id = 2365
        else:
            version = '4'
            filename = candidate['file'].replace('_b26361232.zip', '_v4_b26361232.zip')
            manifest_name = next(n for n in files if n.endswith('.json'))
            manifest = json.loads(files[manifest_name])
            title = manifest['name']
            manifest['version'] = version
            manifest['description'] = manifest['description'].replace(' Candidate for Steam build 26361232.', ' Updated for Steam build 26361232.')
            files[manifest_name] = (json.dumps(manifest, indent=2) + '\n').encode()
            native = candidate['native']
            note = ('Vanilla Multitarget was confirmed working through DMM 4.6.4 on Steam build 26361232, '
                    '2 November 2026. Classic and Caites passed current-data/style/host checks; '
                    'they were not separately replaytested on this build. Their distinct UI payloads are unchanged.')
            body = (f'{title}\nRelease {version} | Steam build 26361232\n\n'
                    'Choose exactly ONE Healthbar variant. Close the game, unmount/remove the previous version, '
                    'then import this complete ZIP into DMM and enable the data mod. ')
            body += ('Also enable the included Healthbar Multitarget in ASI Mods. No separate helper download.\n'
                     if native else 'Classic Single Target needs no Healthbar helper.\n')
            body += ('Keep DMMs existing ASI loader. Remove old manual Healthbar/NativeFix copies; mount and restart. '
                     'Vanilla/Classic use framed bars; Caites uses compact thin bars. Bars use native cleanup on death.\n\n')
            body += ('Only the native helper gains address-shift resilience. '+COMPATIBILITY+'\n' if native else '')
            body += ('The data/UI package still targets Steam build 26361232 and may need rebuilding after later updates. '
                     'Conflicting changes to the same skill or subtitletagview HTML/CSS remain unsupported.\n\n'+note+'\n\n'
                     'Credits: Blablup / gutzufuss1477; Caites - Persistent HP Bars (Nexus 2409).\n')
            files['README.txt'] = body.encode()
            report = json.loads(files['BUILD_VALIDATION.txt'])
            report['release_validation'] = note
            files['BUILD_VALIDATION.txt'] = (json.dumps(report, indent=2) + '\n').encode()
            if native:
                helper = files['Healthbar Multitarget.asi']
            nexus_id = 2406
        if kind != 'healthbar':
            files['README.txt'] = (f'{title}\nRelease {version} | Tested baseline Steam build 26361232\n\n'
                                   + INSTALL_ASI + '\n' + note + '\n\n' + COMPATIBILITY + '\n').encode()
        for name, data in files.items():
            if name.endswith('.asi'):
                assert sha(data) in approved['native_sha256'].values()
        # No functional bytes may change during promotion.
        editable = {'README.txt', 'BUILD_VALIDATION.txt'} | {n for n in original if n.endswith('.json')}
        assert set(files) == set(original)
        assert all(files[n] == original[n] for n in original if n not in editable)
        destination = args.output / 'nexus' / filename
        pack(destination, files)
        records.append(dict(file=filename, sha256=sha(destination.read_bytes()), bytes=destination.stat().st_size,
                            version=version, kind=kind, nexus_mod_id=nexus_id, title=title,
                            in_game_tested=kind!='healthbar' or 'vanilla_multitarget' in filename))
        if kind != 'healthbar':
            (args.output / 'github' / filename).write_bytes(destination.read_bytes())
    assert len(records) == 5 and helper is not None
    helper_name = 'Healthbar_Multitarget_Helper_v4_b26361232.zip'
    helper_file = args.output / 'github' / helper_name
    pack(helper_file, {'Healthbar Multitarget.asi': helper,
        'LICENSE.txt': (ROOT/'mods/healthbar-always-on/source/native/LICENSE.txt').read_bytes(),
        'README.txt': ('Healthbar v4 native helper only, for developers/manual setups.\n'
            'Normal users: download ONE complete DMM ZIP from https://www.nexusmods.com/crimsondesert/mods/2406?tab=files\n'
            'The helper is already bundled in both Multitarget ZIPs. This archive alone is not a complete Healthbar mod.\n\n'
            + COMPATIBILITY + '\nThe data/UI packages remain build-specific.\n').encode()})
    github_helper = dict(file=helper_name, sha256=sha(helper_file.read_bytes()), bytes=helper_file.stat().st_size)
    checksums = ''.join(f"{r['sha256']}  {r['file']}\n" for r in records+[github_helper])
    for folder in ('nexus', 'github'):
        (args.output / folder / 'SHA256SUMS.txt').write_text(checksums)
    report = dict(steam_build='26361232', functional_payloads_unchanged=True,
                  gameplay_confirmed=approved['gameplay_confirmed'], files=records, github_helper=github_helper,
                  excluded_candidates=['mining', 'speed'], healthbar_data_build_specific=True)
    (args.output / 'RELEASE_VALIDATION.json').write_text(json.dumps(report,indent=2)+'\n')
    print('PASS: five Nexus ZIPs and three GitHub ZIPs; tested binary/data payloads unchanged.')

if __name__ == '__main__':
    main()
