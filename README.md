# Crimson Desert â€” Mods

Mods by gutzufuss1477 / Blablup. Bank Refresh 1.3.1 was confirmed on **5 November 2026** for **Steam build 26423880**. Alden and Steelheart updated on **4 November 2026**; both Mount Stats variants and Healthbar updated on **2 November 2026** for **Steam build 26361232**.

| Mod / variant | Release | Current-build status |
|---|---|---|
| [Bank Refresh â€” Gold and Bonds](mods/faster-bank-refresh-gold-and-bonds/) | 1.3.1 | Confirmed in game on Steam 26423880 |
| [All Mounts Level 5 All Stats](mods/real-infinite-horse-stamina-with-optional-lvl5-speed-for-all-hor/) | 13 | Gameplay-confirmed; adaptive ASI |
| [Healthbar â€” three variants](mods/healthbar-always-on/) | 4 | Vanilla Multitarget gameplay-confirmed; Classic/Caites data/style/host validated |
| [Steelheart Horseshoes](mods/real-infinite-horse-stamina-with-optional-lvl5-speed-for-all-hor/) | 10 | Field-level patch compatible with original Equip All Armor; combined gameplay confirmed |
| [Alden Shop](mods/alden-all-in-one-progression-shop/) | 17.3 | Original shop tab with native stocks/prices; nine tabs; gameplay confirmed; Better Inventory / Equip All Armor compatibility retained |
| [Mining Helmet](mods/mining-helmet-always-on/) | Published 1.3.2 for previous build | New 1.4.0 source/candidate awaiting current gameplay confirmation |
| Mount Speed Only | 13 | 1,135 host checks and current structural checks passed; separate gameplay confirmation pending |

[Bank Refresh 1.3.1 download](https://github.com/gutzufuss1477/Crimson-Desert-Mods/releases/tag/bank-v1.3.1-b26423880) · [Steam 26361232 release and checksums](https://github.com/gutzufuss1477/Crimson-Desert-Mods/releases/tag/mods-b26361232). Full Healthbar data/UI ZIPs are on [Nexus](https://www.nexusmods.com/crimsondesert/mods/2406?tab=files); GitHub hosts only its helper, source and recipes.

[Alden v17.3 release notes and checksums](releases/alden-v17.3-b26361232.md) Â· [Complete Alden download on Nexus](https://www.nexusmods.com/crimsondesert/mods/2066?tab=files&file_id=19550). Alden's source, catalog recipe and localization labels are in Git; complete game-data/UI overlays remain Nexus-only.

[Steelheart v10 release notes and checksums](releases/steelheart-v10-b26361232.md) Â· [Steelheart download on Nexus](https://www.nexusmods.com/crimsondesert/mods/2365?tab=files&file_id=19468). All Stats and Speed Only remain v13.

## Installation

Import complete ZIPs into DMM. Enable native plugins under **ASI Mods**, then mount. Bundled INIs are imported and deployed too; preserve existing settings when updating. Bank and both Mount Stats variants also support manual installation with an x64 ASI loader and do not require DMM. Avoid duplicate manual/manager copies; keep the existing loader and DmmRuntime.asi.

Healthbar: choose **one** complete variant, enable its data mod and the bundled helper for either Multitarget style, mount and restart. DMM 4.6.4 import/deployment was verified. Never combine All Stats and Speed Only.

Alden v17.3: import the complete data ZIP and enable the included **Alden Category Tabs.asi** in DMM's ASI Mods section. Update both components together. Completely replace previous Alden packages/helpers so obsolete UI files are removed, then mount and restart. Original Better Inventory 3.3 is optional and no load order is required between these two mods. DMM 4.6.4 is the supported baseline. Alden also requires the current [Microsoft Visual C++ v14 Redistributable (x64)](https://aka.ms/vc14/vc_redist.x64.exe).

## Game updates

The ASI now finds validated code structures instead of relying on fixed addresses or one executable hash. It may keep working after game updates that only move code. Changed code/layouts or ambiguous matches are refused before hooks are installed. Future compatibility is not guaranteed.

This applies to the newly released Bank, All Stats and Speed Only ASIs and Healthbar's native helper. Healthbar data/UI still target the listed build. Alden v17.3's data/UI and category helper target build 26361232; its helper uses exact executable/UI guards. Alden's v17.3 prices and Steelheart v10 use DMM field edits, preserving original Equip All Armor changes when item record sizes move. They still require compatible game data; DLC data remain unverified.

Mining is not part of this release. The obsolete Bank Save Patcher remains excluded.

See [build and validation](tooling/README.md), [distribution](DISTRIBUTION.md), [security reporting](SECURITY.md) and [license scope](LICENSES.md). Game inputs, full game tables/UI, executables and private diagnostics stay out of Git.
