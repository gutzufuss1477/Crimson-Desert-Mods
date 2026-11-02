# Horse stamina and mount stats

| Download | Version | Baseline / status |
|---|---|---|
| All Mounts Level 5 All Stats | 13 | Steam 26361232; gameplay confirmed 2 November 2026 |
| Speed Only (previous release) | 12 | Steam 26329202; new current-build candidate not released yet |
| Steelheart Horseshoes | 9, unchanged | Current relevant inputs identical; guarded patch verified |

All Stats changes speed, acceleration, turning and jump. Speed Only changes movement speed alone. Use **one** mount ASI. Steelheart raises the equipped horseshoes' stamina-regeneration buff from 4 to 20 and is optional.

[Nexus downloads](https://www.nexusmods.com/crimsondesert/mods/2365?tab=files) · [Current release](https://github.com/gutzufuss1477/Crimson-Desert-Mods/releases/tag/mods-b26361232)

## Install

For All Stats, close the game, remove older/manual duplicate ASIs, import the complete v13 ZIP in DMM under ASI Mods, enable and mount. DMM 4.6.4 deployment was verified. Keep the loader and DmmRuntime.asi.

Manual alternative: copy AllMountsLvL5AllStats.asi into bin64 with an x64 ASI loader. Neither DMM nor Steelheart is required by the ASI. Disable/remove it with the game closed.

For Steelheart, import its unchanged v9 ZIP into DMM, enable and mount. Equip the horseshoes. Conflicting equip-buff edits are unsupported.

## Compatibility and validation

All Stats v13: The ASI now finds validated code structures instead of relying on fixed addresses or one executable hash. It may keep working after game updates that only move code. Changed code/layouts or ambiguous matches are refused before hooks are installed. Future compatibility is not guaranteed.

The same claim does not apply to the old Speed Only release or Steelheart's data patch. All Stats passed 1,135 host checks and structural resolver checks; its released binary matches the gameplay-tested candidate. Startup status: %LOCALAPPDATA%/CrimsonDesertMods/Mount_AllStats.status.txt.

Build All Stats with tooling/build_allstats_204.cmd. Speed Only's adaptive source and host tests are retained as a candidate; no new Speed Only binary is included in this release. See [tooling](../../tooling/README.md) and nexus.json.
