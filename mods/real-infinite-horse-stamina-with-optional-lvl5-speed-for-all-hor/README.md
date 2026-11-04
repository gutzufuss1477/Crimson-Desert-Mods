# Horse stamina and mount stats

| Download | Version | Baseline / status |
|---|---|---|
| All Mounts Level 5 All Stats | 13 | Steam 26361232; gameplay confirmed 2 November 2026 |
| Speed Only | 13 | Steam 26361232; host/structural checks passed, separate gameplay confirmation pending |
| Steelheart Horseshoes | 10 | Game 2.04.05 / Steam 26361232; field-level patch, combined gameplay confirmed 4 November 2026 |

All Stats changes speed, acceleration, turning and jump. Speed Only changes movement speed alone. Use **one** mount ASI. Steelheart raises the equipped horseshoes' stamina-regeneration buff from 4 to 20 and is optional.

[Nexus downloads](https://www.nexusmods.com/crimsondesert/mods/2365?tab=files) · [Current release](https://github.com/gutzufuss1477/Crimson-Desert-Mods/releases/tag/mods-b26361232)

## Install

For either mount ASI, close the game, remove older/manual duplicate ASIs, import the complete v13 ZIP in DMM under ASI Mods, enable and mount. DMM 4.6.4 deployment was verified with All Stats. Keep the loader and DmmRuntime.asi. Both ZIPs use the same simple ASI-only layout; neither needs an INI.

Manual alternative: copy either AllMountsLvL5AllStats.asi or AllMountsLvL5Speed.asi into bin64 with an x64 ASI loader. Neither DMM nor Steelheart is required by the ASI. Disable/remove it with the game closed.

For Steelheart v10, close the game and unmount in **DMM 4.6.4 or later**. Replace/disable the old Steelheart patch, import `Steelheart_Horseshoes.zip`, enable **Steelheart Horseshoes - 20 Stamina Regen**, mount and restart. Equip the horseshoes. Keep only one Steelheart patch active. No ASI is required. Other edits to the same stamina-regeneration buff may conflict.

[Download Steelheart v10](https://www.nexusmods.com/crimsondesert/mods/2365?tab=files&file_id=19468) · [Steelheart release notes](../../releases/steelheart-v10-b26361232.md) · [Changelog](CHANGELOG.md)

## Compatibility and validation

Steelheart v10 fixes the outdated-patch warning with original Character Creator Enhanced — Equip All Armor. It targets item 1000594's stamina-regeneration buff field instead of a fixed file position. The effect remains level 4 to 20. Armor changes are preserved in either application order; Character Creator and Alden remain optional, separate mods.

The author confirmed Alden, Steelheart, original Character Creator Enhanced 9.1.32 / Equip All Armor and original Better Inventory 3.3 together in DMM and in game on 4 November 2026 without errors. The final packages preserve the confirmed field targets, effects and runtime payloads; only release metadata and readmes changed. The exact final ZIPs were not separately run in game. DLC installations/data remain unverified.

All Stats and Speed Only remain v13; neither ASI changed in this update.

Both mount ASIs v13: The ASI now finds validated code structures instead of relying on fixed addresses or one executable hash. It may keep working after game updates that only move code. Changed code/layouts or ambiguous matches are refused before hooks are installed. Future compatibility is not guaranteed.

The same claim does not apply to Steelheart's data patch. Each mount variant passed 1,135 host checks and structural resolver checks. Both released binaries match their validated candidates; All Stats is gameplay-confirmed, while Speed Only's separate gameplay test on this build is pending. Speed Only shares All Stats' runtime except for the speed-only filter and status filename. Startup status: %LOCALAPPDATA%/CrimsonDesertMods/Mount_AllStats.status.txt or Mount_Speed.status.txt.

Build All Stats with tooling/build_allstats_204.cmd and Speed Only with tooling/build_speed_204.cmd. Speed Only v13 is a supplemental asset on the current GitHub release, with SHA256SUMS-SpeedOnly.txt; the original release assets and tag are unchanged. See [tooling](../../tooling/README.md) and nexus.json.
