# Horse stamina and mount stats

| Download | Version | Baseline / status |
|---|---|---|
| All Mounts Level 5 All Stats | 13 | Steam 26361232; gameplay confirmed 2 November 2026 |
| Speed Only | 13 | Steam 26361232; host/structural checks passed, separate gameplay confirmation pending |
| Steelheart Horseshoes | 9, unchanged | Current relevant inputs identical; guarded patch verified |

All Stats changes speed, acceleration, turning and jump. Speed Only changes movement speed alone. Use **one** mount ASI. Steelheart raises the equipped horseshoes' stamina-regeneration buff from 4 to 20 and is optional.

[Nexus downloads](https://www.nexusmods.com/crimsondesert/mods/2365?tab=files) · [Current release](https://github.com/gutzufuss1477/Crimson-Desert-Mods/releases/tag/mods-b26361232)

## Install

For either mount ASI, close the game, remove older/manual duplicate ASIs, import the complete v13 ZIP in DMM under ASI Mods, enable and mount. DMM 4.6.4 deployment was verified with All Stats. Keep the loader and DmmRuntime.asi. Both ZIPs use the same simple ASI-only layout; neither needs an INI.

Manual alternative: copy either AllMountsLvL5AllStats.asi or AllMountsLvL5Speed.asi into bin64 with an x64 ASI loader. Neither DMM nor Steelheart is required by the ASI. Disable/remove it with the game closed.

For Steelheart, import its unchanged v9 ZIP into DMM, enable and mount. Equip the horseshoes. Conflicting equip-buff edits are unsupported.

## Compatibility and validation

Both mount ASIs v13: The ASI now finds validated code structures instead of relying on fixed addresses or one executable hash. It may keep working after game updates that only move code. Changed code/layouts or ambiguous matches are refused before hooks are installed. Future compatibility is not guaranteed.

The same claim does not apply to Steelheart's data patch. Each mount variant passed 1,135 host checks and structural resolver checks. Both released binaries match their validated candidates; All Stats is gameplay-confirmed, while Speed Only's separate gameplay test on this build is pending. Speed Only shares All Stats' runtime except for the speed-only filter and status filename. Startup status: %LOCALAPPDATA%/CrimsonDesertMods/Mount_AllStats.status.txt or Mount_Speed.status.txt.

Build All Stats with tooling/build_allstats_204.cmd and Speed Only with tooling/build_speed_204.cmd. Speed Only v13 is a supplemental asset on the current GitHub release, with SHA256SUMS-SpeedOnly.txt; the original release assets and tag are unchanged. See [tooling](../../tooling/README.md) and nexus.json.
