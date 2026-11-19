# Crimson Desert mods

Mods by Blablup / gutzufuss1477 for Steam build **26776744** (EXE 1.0.0.3732), 19 November 2026.

| Mod | Version | Current-build checks |
|---|---|---|
| [Alden Shop](mods/alden-all-in-one-progression-shop/) | DLC 17.4.12 / No DLC 17.3.9 | DLC and No DLC category switching and catalog prices confirmed in game with Charting the Unknown installed. Both full ZIPs rebuilt byte-for-byte from the release source and local game inputs. |
| [Bank Refresh](mods/faster-bank-refresh-gold-and-bonds/) | 1.4.1 | The updated bank component and native lookup passed the executable, timer and load tests. A separate gameplay check of both timers on this build is pending. |
| [Mount stat display / Steelheart](mods/real-infinite-horse-stamina-with-optional-lvl5-speed-for-all-hor/) | v15 / v11 | Steelheart v11 mounted with Alden. The equipped buff in the merged item table was verified at 20. Both existing v15 stat-display ASIs passed the current executable checks. |
| [Mining Helmet](mods/mining-helmet-always-on/) | 1.5.0 | The released ASI passed all 36 native checks. The ore group and 375 relevant records match. |
| [Healthbar, all three styles](mods/healthbar-always-on/) | v5 | All three v5 packages match the expected current skill and UI payloads. Character edits and the shared Multitarget helper passed the checks. |

[Release notes and Nexus downloads](releases/mods-b26776744.md) · [GitHub release](https://github.com/gutzufuss1477/Crimson-Desert-Mods/releases/tag/mods-b26776744) · [Checksums](releases/mods-b26776744-SHA256SUMS.txt).

## Installation

Close the game, unmount and replace previous copies in DMM. Import complete ZIPs; enable native helpers under **ASI Mods**, mount and restart. Bundled INIs are imported too; back up custom settings. Keep the working loader and DmmRuntime.asi. Avoid duplicate manual and manager copies. Bank, Mining and both Mount ASIs also support manual installation with an x64 ASI loader.

Use All Stats OR Speed Only. Healthbar: choose ONE complete style and enable the included helper for either Multitarget variant; disable it for Classic. Alden: choose DLC OR No DLC and import one complete ZIP and enable the shop and its included category helper. DLC requires Charting the Unknown. Alden also requires the [Microsoft Visual C++ v14 x64 runtime](https://aka.ms/vc14/vc_redist.x64.exe). Old Better Inventory 3.3 compiled UI is not supported on this build.

## Future updates

Bank, Mining, Mount and Healthbar's native helper resolve validated code structures. Compatible address shifts may survive a game update; changed layouts/functions still require updates. Healthbar's data/UI and Alden's exact executable/UI guards remain build-dependent. Steelheart's field-level patch preserves unrelated item edits but still needs compatible game data. No mod has guaranteed future compatibility.

Full Alden and Healthbar data/UI packages are distributed on Nexus; GitHub contains owned source, recipes, hashes and eligible native-only assets. The obsolete Bank Save Patcher and experimental Better Mount Movement are excluded.

See [build instructions](tooling/README.md), [distribution](DISTRIBUTION.md), [security reporting](SECURITY.md) and [license scope](LICENSES.md).
