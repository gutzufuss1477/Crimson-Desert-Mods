# Build and validation

## Steam build 26361232 release and remaining candidates

Bank, All Stats and Vanilla Multitarget were confirmed working through DMM 4.6.4
on build 26361232 on 2 November 2026. Mining remains a candidate. Speed Only v13 is also published after 1,135 host checks and a fresh current-build resolver check; its separate gameplay confirmation is pending.
Classic/Caites have current data/style/host validation but no separate current-build gameplay test. Published releases below retain
their historical validation claims; rebuilding this source does not reproduce
those historical binaries.

The five native build commands listed below now produce stable ASI filenames.
They share `native/resolver.hpp` and `native/profiles_26361232.hpp`. Build and run
the additional resolver checks against a locally installed or copied executable:

```bat
tooling\native\build_tests.cmd
tooling\native\out\resolver_tests.exe "<path-to-build-26361232>\CrimsonDesert.exe"
tooling\native\out\resolver_tests.exe "<path-to-build-26329202>\CrimsonDesert.exe" previous
```

The test maps a private, non-executable copy. It validates all four resolver plans,
changed code, ambiguous matches, call/global references, RTTI/vtable identity,
our Mount/Healthbar hook coexistence, malformed input and rebased data pointers.
It does not execute game code. On the previous build, Bank must refuse the old
object layout; Mount, Mining and the Healthbar helper still resolve. The new
Healthbar **data packages** are specific to build 26361232.

Resolution uses unique normalized code fingerprints and structural references,
not fixed addresses or an unconditional bypass of the executable hash. Function
address displacements may move; opcodes, field offsets and constants must match.
Bank was adapted to the new records/count fields at 0x130/0x138. A changed or
ambiguous plan disables that ASI before hooks are installed. Other mods changing
the same validated functions can still prevent resolution. This improves
resilience to address shifts, not arbitrary game changes.

Build all three Healthbar data variants from fresh vanilla inputs, then package:

```powershell
$env:PYTHONPATH = 'tooling'
python -m crimson_mod_tools.build_204 --game-version b26361232 --inputs <vanilla-root> --output dist/healthbar-data-b26361232 --mods healthbar
python -m crimson_mod_tools.verify_data_204 --game-version b26361232 --inputs <vanilla-root> --output dist/healthbar-data-b26361232 --expected-count 3
python tooling/package_candidates_b26361232.py --healthbar-data dist/healthbar-data-b26361232 --output dist/candidate-b26361232
```

`recipes/b26361232/native-candidates.json` records hashes of the validated local
binaries. A different rebuild must be validated before replacing those hashes.
The candidate packager produces four standalone ASI ZIPs and three complete
Healthbar ZIPs. The two Multitarget variants include `Healthbar Multitarget.asi`;
Classic Single Target has no helper. No loader DLL is bundled. Existing INI
settings should be preserved. AllStats and Speed Only are alternatives.

At the initial build-26361232 release, Alden v16 and Steelheart v9 retained their
byte-identical relevant inputs and all 503 patch preimages. Alden has since been
updated to v17 with an expanded catalog and localized tabs; see below.
Healthbar keeps the three existing UI styles and updates the skill overlay for
the current ability ID. Full game-data/UI packages remain local/Nexus-only under
the repository distribution policy.

## Published 2.04.02 releases (historical)

Published native release: Crimson Desert 2.04.02, Steam build 26329202. Build with Visual Studio C++ x64 tools and the Windows SDK:

```bat
mods\mining-helmet-always-on\source\src\build.cmd
mods\faster-bank-refresh-gold-and-bonds\source\src\build.cmd
tooling\build_allstats_204.cmd
tooling\build_speed_204.cmd
```

Each command runs its host tests. Mining covers configuration, lifecycle, camera/material handling and ore range; Bank covers scheduling, hooks and initialization; each mount variant has 1,135 checks. Exact-build native bytes/vtable slots were checked against the target executable. The author confirmed Mining, Bank and AllStats working in game on 31 October 2026. Speed-only's separate gameplay test is pending.

## Alden v17 — localized category shop

The complete v17 package is published on [Nexus](https://www.nexusmods.com/crimsondesert/mods/2066?tab=files&file_id=19088).
It contains 567 items, eight category tabs and 15 language variants, and requires
both the DMM data mod and the bundled ASI. Build the native helper and its
callback tests with Visual Studio C++ x64 tools:

```bat
mods\alden-all-in-one-progression-shop\source\native\build.cmd
```

The source includes the fixed executable/UI hashes used by the published
package. The native tests cover loader/UI thread handoff, category membership,
purchase-record identity, shop reopening, other-merchant isolation and cleanup.
The source's `translations.json` and `aldencategorytabs.css` define the new labels
and their layout.

`recipes/alden-v17-b26361232/` records the exact manifest, ordered 567-item stock
list, category assignments and functional payload hashes. The complete game
tables and compiled UI overlays remain local/Nexus-only. The historical
`build_204 --mods alden` workflow below reconstructs the earlier 379-item catalog;
it does not generate the v17 compiled UI or localization additions.

See [release notes and checksums](../releases/alden-v17-b26361232.md) for the scope
of the author-confirmed game tests and the exact published package.

## Historical data recipes (Alden v16 / Steelheart v9)

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

## Healthbar v3

Healthbar is published separately for the same game build. Keep the extracted `0012/ui/xml/gamemain/play` UI files as well as the current `0008` tables. Build the helper with Visual Studio C++ x64 tools:

```bat
mods\healthbar-always-on\source\native\build.cmd
```

Build and validate the three data variants, then combine them with the validated helper:

```powershell
$env:PYTHONPATH = 'tooling'
python -m crimson_mod_tools.build_204 --game-version 2.04.02 --inputs <vanilla-root> --output dist/healthbar-data --mods healthbar
python -m crimson_mod_tools.verify_data_204 --game-version 2.04.02 --inputs <vanilla-root> --output dist/healthbar-data --expected-count 3
python -m crimson_mod_tools.package_healthbar_204 --data-dir dist/healthbar-data --native mods/healthbar-always-on/source/native/out/Healthbar_Multitarget_NativeFix2_2.04.02_b26329202.asi --output dist/healthbar-complete
```

The combined packager checks the native and UI hashes, preserves all data payloads and character patches, and places the native helper at the ZIP root with a readable filename. Each ZIP has one real JSON manifest; diagnostic reports use `.txt`. There are no nested archives or bundled loader DLLs. An alternate compiler may produce different native bytes; validate them before changing the accepted hash.

Vanilla Multitarget and Classic were confirmed in game; Caites preserves the previously confirmed compact UI payloads and native patch. DMM 4.5.1 imported the combined package, deployed the helper and activated it in-game. These three full game-data/UI packages are distributed on Nexus. GitHub contains the source, recipes, helper and hashes.

## Promote validated b26361232 payloads

Run python tooling/package_release_b26361232.py --candidates dist/candidate-b26361232 --output dist/release-b26361232. The approval record limits publication to Bank, All Stats and three Healthbar variants. Only README/validation metadata and Healthbar manifest version text change; all native/data/UI payloads must remain identical. The GitHub folder excludes complete game-data/UI packages.

Speed Only was subsequently authorized as a supplemental release. Run `python tooling/package_speed_release_b26361232.py --candidates dist/candidate-b26361232 --output dist/release-speed-b26361232`. Its separate approval records the passed technical checks and the pending gameplay test. The packager preserves the validated ASI exactly and emits a ZIP plus `SHA256SUMS-SpeedOnly.txt`, without replacing any existing release artifact.
