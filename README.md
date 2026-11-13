# Crimson Desert — Mods

Mods by gutzufuss1477 / Blablup. Updated **13 November 2026** for **Steam build 26636972**, EXE 1.0.0.3663. DMM 5.0.0 was used for the current setup.

| Mod | Release | Current-build validation |
|---|---|---|
| [Bank Refresh — Gold and Bonds](mods/faster-bank-refresh-gold-and-bonds/) | 1.4.0 | Author reports working in game; timer paths technically checked |
| [Mining Helmet Always On](mods/mining-helmet-always-on/) | 1.5.0 | Ore highlighting confirmed in game |
| [Mount All Stats / Speed Only](mods/real-infinite-horse-stamina-with-optional-lvl5-speed-for-all-hor/) | 15 | Display/cache changes only; All Stats confirmed, Speed Only technically checked |
| [Healthbar — three variants](mods/healthbar-always-on/) | 5 | Vanilla Multitarget confirmed; Classic and Caites technically checked |
| [Alden Shop](mods/alden-all-in-one-progression-shop/) | DLC 17.4.6 / No DLC 17.3.7 | DLC shop/categories confirmed; No DLC technically checked |
| [Steelheart Horseshoes](mods/real-infinite-horse-stamina-with-optional-lvl5-speed-for-all-hor/) | 10, unchanged | Current data checks passed; separate new gameplay test pending |

**Mount scope correction:** the ASIs set the displayed stat gauges to level 5. Their UI/cache implementation does not establish a change to actual movement/physics or stamina. Earlier claims of actual speed/acceleration/turn/jump boosts were unsupported. Steelheart separately changes the equipped horseshoes' stamina-regeneration buff from 4 to 20.

[Current release notes](releases/mods-b26636972.md) · [GitHub native downloads](https://github.com/gutzufuss1477/Crimson-Desert-Mods/releases/tag/mods-b26636972) · [Full payload hashes](releases/mods-b26636972-payloads.json).

## Installation

Close the game, unmount and replace previous copies in DMM. Import complete ZIPs; enable native helpers under **ASI Mods**, mount and restart. Bundled INIs are imported too; back up custom settings. Keep the working loader and DmmRuntime.asi. Avoid duplicate manual and manager copies. Bank, Mining and both Mount ASIs also support manual installation with an x64 ASI loader.

Use All Stats OR Speed Only. Healthbar: choose ONE complete style and enable the included helper for either Multitarget variant; disable it for Classic. Alden: choose DLC OR No DLC and update the data, price component and category helper together. DLC requires Charting the Unknown. Alden also requires the [Microsoft Visual C++ v14 x64 runtime](https://aka.ms/vc14/vc_redist.x64.exe). Old Better Inventory 3.3 compiled UI is not supported on this build.

## Future updates

Bank, Mining, Mount and Healthbar's native helper resolve validated code structures. Compatible address shifts may survive a game update; changed layouts/functions still require updates. Healthbar's data/UI and Alden's exact executable/UI guards remain build-dependent. Steelheart's field-level patch preserves unrelated item edits but still needs compatible game data. No mod has guaranteed future compatibility.

Full Alden and Healthbar data/UI packages are distributed on Nexus; GitHub contains owned source, recipes, hashes and eligible native-only assets. The obsolete Bank Save Patcher and experimental Better Mount Movement are excluded.

See [build instructions](tooling/README.md), [distribution](DISTRIBUTION.md), [security reporting](SECURITY.md) and [license scope](LICENSES.md).
