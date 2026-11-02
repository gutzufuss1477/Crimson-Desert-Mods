# Crimson Desert — Mods

Mods by gutzufuss1477 / Blablup. Bank, Mount All Stats and Healthbar updated on **2 November 2026** for **Steam build 26361232**.

| Mod / variant | Release | Current-build status |
|---|---|---|
| [Bank Refresh — Gold and Bonds](mods/faster-bank-refresh-gold-and-bonds/) | 1.3.0 | Both timers gameplay-confirmed; adaptive ASI |
| [All Mounts Level 5 All Stats](mods/real-infinite-horse-stamina-with-optional-lvl5-speed-for-all-hor/) | 13 | Gameplay-confirmed; adaptive ASI |
| [Healthbar — three variants](mods/healthbar-always-on/) | 4 | Vanilla Multitarget gameplay-confirmed; Classic/Caites data/style/host validated |
| [Steelheart Horseshoes](mods/real-infinite-horse-stamina-with-optional-lvl5-speed-for-all-hor/) | 9, unchanged | Relevant data identical; guarded patch verified |
| [Alden Shop](mods/alden-all-in-one-progression-shop/) | 16, unchanged | Relevant data identical; 502 patch preimages verified |
| [Mining Helmet](mods/mining-helmet-always-on/) | Published 1.3.2 for previous build | New 1.4.0 source/candidate awaiting current gameplay confirmation |
| Mount Speed Only | Published 12 for previous build | New 13 source/candidate awaiting separate gameplay confirmation |

[Current release and checksums](https://github.com/gutzufuss1477/Crimson-Desert-Mods/releases/tag/mods-b26361232). Full Healthbar data/UI ZIPs are on [Nexus](https://www.nexusmods.com/crimsondesert/mods/2406?tab=files); GitHub hosts only its helper, source and recipes.

## Installation

Import complete ZIPs into DMM. Enable native plugins under **ASI Mods**, then mount. Bundled INIs are imported and deployed too; preserve existing settings when updating. Bank and All Stats also support manual installation with an x64 ASI loader and do not require DMM. Avoid duplicate manual/manager copies; keep the existing loader and DmmRuntime.asi.

Healthbar: choose **one** complete variant, enable its data mod and the bundled helper for either Multitarget style, mount and restart. DMM 4.6.4 import/deployment was verified. Never combine All Stats and Speed Only.

## Game updates

The ASI now finds validated code structures instead of relying on fixed addresses or one executable hash. It may keep working after game updates that only move code. Changed code/layouts or ambiguous matches are refused before hooks are installed. Future compatibility is not guaranteed.

This applies to the newly released Bank/All Stats ASIs and Healthbar's native helper. Healthbar data/UI still target the listed build. Alden and Steelheart remain data mods; their current inputs were verified unchanged, not made universally compatible.

Mining and Speed Only are not part of this release. The obsolete Bank Save Patcher remains excluded.

See [build and validation](tooling/README.md), [distribution](DISTRIBUTION.md), [security reporting](SECURITY.md) and [license scope](LICENSES.md). Game inputs, full game tables/UI, executables and private diagnostics stay out of Git.
