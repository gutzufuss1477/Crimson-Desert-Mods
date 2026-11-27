# Crimson Desert mods

Community mods by Blablup / gutzufuss1477. Tested in game on **2.06.00**, Steam build **26949954**, with DMM **5.3.2**.

| Mod | Current release |
|---|---|
| [Alden Shop](mods/alden-all-in-one-progression-shop/) | DLC 17.4.14 · No DLC 17.3.11 |
| [Mount stats and Steelheart](mods/real-infinite-horse-stamina-with-optional-lvl5-speed-for-all-hor/) | All Stats and Speed Only v16 · Steelheart v12 |
| [Healthbar](mods/healthbar-always-on/) | Classic Single Target, Vanilla Multitarget and Caites Multitarget v6 |
| [Mining Helmet](mods/mining-helmet-always-on/) | 1.6.0 |
| [Bank Refresh](mods/faster-bank-refresh-gold-and-bonds/) | 1.4.2 |

[Release notes and Nexus downloads](releases/mods-b26949954.md) · [GitHub release](https://github.com/gutzufuss1477/Crimson-Desert-Mods/releases/tag/mods-b26949954) · [Checksums](releases/mods-b26949954-SHA256SUMS.txt).

## Installation

Close the game, unmount and replace previous copies in DMM. Import complete ZIPs; enable native helpers under **ASI Mods**, mount and restart. Bundled INIs are imported too; back up custom settings. Keep the working loader and DmmRuntime.asi. Avoid duplicate manual and manager copies. Bank, Mining and both Mount ASIs also support manual installation with an x64 ASI loader.

Use All Stats OR Speed Only. Healthbar: choose ONE complete style and enable the included helper for either Multitarget variant; disable it for Classic. Alden: choose DLC OR No DLC and import one complete ZIP and enable the shop and its included category helper. DLC requires Charting the Unknown. Alden also requires the [Microsoft Visual C++ v14 x64 runtime](https://aka.ms/vc14/vc_redist.x64.exe). Old Better Inventory 3.3 compiled UI is not supported on this build.

## Future updates

Bank, Mount and Healthbar's native helper resolve validated code structures. Compatible address shifts may survive a game update; changed layouts/functions still require updates. Healthbar's data/UI and the exact-build checks in Alden and Mining Helmet remain build-dependent. Steelheart's field-level patch preserves unrelated item edits but still needs compatible game data. No mod has guaranteed future compatibility.

Full Alden and Healthbar data/UI packages are distributed on Nexus; GitHub contains owned source, recipes, hashes and eligible native-only assets. The obsolete Bank Save Patcher and experimental Better Mount Movement are excluded.

See [build instructions](tooling/README.md), [distribution](DISTRIBUTION.md), [security reporting](SECURITY.md) and [license scope](LICENSES.md).
