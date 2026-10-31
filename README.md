# Crimson Desert — Mods

Mods by gutzufuss1477 / Blablup. Updated **31 October 2026** for **game 2.04.02 / Steam build 26329202**.

| Mod / variant | Version | Install | Download |
|---|---|---|---|
| [Mining Helmet Always On](mods/mining-helmet-always-on/) | 1.3.2 | ASI loader | [Nexus](https://www.nexusmods.com/crimsondesert/mods/3586) |
| [Faster Bank Refresh — Gold and Bonds](mods/faster-bank-refresh-gold-and-bonds/) | 1.2.2 | ASI loader | [Nexus](https://www.nexusmods.com/crimsondesert/mods/3669) |
| [All Mounts LvL 5 All Stats / Speed-only](mods/real-infinite-horse-stamina-with-optional-lvl5-speed-for-all-hor/) | 12 | ASI loader | [Nexus](https://www.nexusmods.com/crimsondesert/mods/2365) |
| [Steelheart Horseshoes](mods/real-infinite-horse-stamina-with-optional-lvl5-speed-for-all-hor/) | 9 | DMM 4.5.1+ | [Nexus](https://www.nexusmods.com/crimsondesert/mods/2365) |
| [Alden All-In-One Progression Shop](mods/alden-all-in-one-progression-shop/) | 16 | DMM 4.5.1+ | [Nexus](https://www.nexusmods.com/crimsondesert/mods/2066) |
| [Healthbar: Vanilla Multitarget / Classic Single Target / Caites](mods/healthbar-always-on/) | 3 | DMM 4.5.1; helper bundled for Multitarget | [Nexus](https://www.nexusmods.com/crimsondesert/mods/2406?tab=files) |

Mining Helmet, Bank Refresh, AllStats, Alden and Steelheart were confirmed working in-game by the author. Speed-only passed static/host/loader checks and uses the AllStats hook with a narrower stat filter; its separate gameplay test is pending.

[GitHub release, native/Steelheart downloads and all six package checksums](https://github.com/gutzufuss1477/Crimson-Desert-Mods/releases/tag/mods-2.04.02-b26329202). The ready-to-install Alden data overlay is available on Nexus; its source and recipes are in this repository. These packages target the exact Steam build above.

## Install

For an ASI, close the game, remove older copies of the same mod and copy the ASI into `Crimson Desert/bin64` beside `CrimsonDesert.exe`. Preserve your existing INI. Keep an existing [x64 ASI loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader); it is not bundled. These ASIs do not require DMM. Never load AllStats and Speed-only together.

For Alden or Steelheart, close the game and unmount in DMM. Import the matching ZIP with DMM 4.5.1 or a compatible newer release, disable the previous version, enable the update and mount. Keep DMM's runtime and loader, then restart the game. See each mod's README for details.

Healthbar v3 restores the two Multitarget modes and keeps the three distinct presentations. Import one complete Nexus ZIP into DMM, enable its data component and (for Multitarget) the included **Healthbar Multitarget** plugin in **ASI Mods**, then mount and restart. No separate helper download or manual file copy is needed. [Healthbar source, validation and installation](mods/healthbar-always-on/).

## Excluded from this update

The obsolete Bank Save Patcher is excluded.

See [build and validation](tooling/README.md), [distribution notes](DISTRIBUTION.md), [security reporting](SECURITY.md) and [license scope](LICENSES.md). Original game inputs, complete game tables, executables, private diagnostics and credentials are excluded from Git.
