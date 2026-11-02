"""Package Speed Only v13 without changing its validated native payload."""
from pathlib import Path
import argparse
import hashlib
import json
import zipfile
from package_candidates_b26361232 import pack
from package_release_b26361232 import COMPATIBILITY

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidates', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    approval = json.loads((ROOT / 'recipes/b26361232/speed-release-approval.json').read_text())
    candidates = json.loads((args.candidates / 'VALIDATION.json').read_text())
    row = next(r for r in candidates['files'] if r['kind'] == 'speed')
    source = args.candidates / row['file']
    assert hashlib.sha256(source.read_bytes()).hexdigest() == row['sha256']
    with zipfile.ZipFile(source) as z:
        assert z.testzip() is None
        files = {n: z.read(n) for n in z.namelist()}
    assert set(files) == {'AllMountsLvL5Speed.asi', 'README.txt'}
    native_hash = hashlib.sha256(files['AllMountsLvL5Speed.asi']).hexdigest()
    assert native_hash == approval['native_sha256']
    assert approval['static_and_host_tests_passed'] and not approval['gameplay_tested']
    assert approval['host_checks'] == 1135
    files['README.txt'] = (
        'All Mounts Level 5 - Speed Only\nRelease 13 | Steam build 26361232\n\n'
        'Changes movement speed only. Acceleration, turning and jump are unchanged.\n'
        'Use Speed Only OR All Stats, never both. Steelheart Horseshoes is optional and separate.\n\n'
        'INSTALL WITH DMM\nClose the game. Disable/remove the old Speed Only or All Stats ASI, '
        'including any manual duplicate. Import this complete ZIP in DMM under ASI Mods, '
        'enable Speed Only and mount. Keep DmmRuntime.asi and the existing ASI loader. '
        'No INI or separate configuration file is needed.\n'
        'Manual alternative: copy AllMountsLvL5Speed.asi beside CrimsonDesert.exe in bin64 '
        'with an x64 ASI loader. DMM is optional; no loader DLL is bundled.\n\n'
        'VALIDATION\n1,135 host register/filter/breakpoint checks and structural resolver checks '
        'passed against Steam build 26361232 on 2 November 2026. The runtime is shared with '
        'gameplay-confirmed All Stats, except for the speed-only filter and status filename. '
        'Separate Speed Only gameplay confirmation on this build is still pending. '
        'Host checks do not execute game code. The native payload is unchanged from the validated candidate.\n\n'
        + COMPATIBILITY + '\n\n'
        'Startup status: %LOCALAPPDATA%\\CrimsonDesertMods\\Mount_Speed.status.txt.\n'
        'Uninstall: close the game, disable/remove in DMM and remount remaining mods. '
        'Remove a manually installed ASI manually.\n'
    ).encode()
    args.output.mkdir(parents=True, exist_ok=True)
    destination = args.output / 'AllMountsLvL5Speed_v13_b26361232.zip'
    pack(destination, files)
    digest = hashlib.sha256(destination.read_bytes()).hexdigest()
    (args.output / 'SHA256SUMS-SpeedOnly.txt').write_text(f'{digest}  {destination.name}\n')
    record = dict(file=destination.name, sha256=digest, bytes=destination.stat().st_size,
                  native_sha256=native_hash, version='13', kind='speed', nexus_mod_id=2365,
                  steam_build='26361232', in_game_tested=False, host_checks=1135,
                  structural_checks_passed=True, functional_payload_unchanged=True)
    (args.output / 'RELEASE_VALIDATION.json').write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps(record, indent=2))


if __name__ == '__main__':
    main()
