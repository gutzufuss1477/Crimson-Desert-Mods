# Build and validation

Current native release: Crimson Desert 2.04.02, Steam build 26329202. Build with Visual Studio C++ x64 tools and the Windows SDK:

```bat
mods\mining-helmet-always-on\source\src\build.cmd
mods\faster-bank-refresh-gold-and-bonds\source\src\build.cmd
tooling\build_allstats_204.cmd
tooling\build_speed_204.cmd
```

Each command runs its host tests. Mining covers configuration, lifecycle, camera/material handling and ore range; Bank covers scheduling, hooks and initialization; each mount variant has 1,135 checks. Exact-build native bytes/vtable slots were checked against the target executable. The author confirmed Mining, Bank and AllStats working in game on 31 October 2026. Speed-only's separate gameplay test is pending.

## Data recipes

Use Python 3.11+ and locally extracted, unmodified game files for the supported build. Keep the `0008/gamedata/binarystaticinfo__/bin` extraction paths. No original game inputs are distributed. The builder rejects unexpected hashes and preserves non-target records.

```powershell
$env:PYTHONPATH = 'tooling'
python -m crimson_mod_tools.build_204 --game-version 2.04.02 --inputs <vanilla-root> --output dist/2.04.02-b26329202 --mods alden steelheart
python -m crimson_mod_tools.verify_data_204 --game-version 2.04.02 --inputs <vanilla-root> --output dist/2.04.02-b26329202 --expected-count 2
python -m unittest discover -s tooling/tests -v
```

Alden retains the ordered 379-item catalog and stock, changes only Store 3101, and validates 502 purchase-price edits. Steelheart changes the equipped horseshoes' stamina-regeneration buff from level 4 to 20. Both passed DMM 4.5.1 mount and author gameplay checks.

## Release packaging

```powershell
python tooling/package_release_20402.py --output dist/release-2.04.02-b26329202
```

The packager requires the approved native binary hashes and every functional payload hash from the two validated data candidates. It updates release documentation without changing the tested payloads and writes six ZIPs plus SHA256SUMS.txt. Candidate documentation may differ after tooling edits; changed functional payload hashes require new validation. A fresh compiler/toolchain can produce different native bytes and needs its own validation.

GitHub hosts the four native ZIPs and the Steelheart manifest-only ZIP. Alden's complete modified table overlay is distributed through Nexus; its recipes/build source and all six package hashes are recorded here. Healthbar is not part of this release. `package_native_release.py` retains the historical 2.04.00 packager.
