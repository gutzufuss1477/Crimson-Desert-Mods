# Crimson Desert — Mods

Mods by gutzufuss1477 / Blablup.

## Native ASI update: game 2.04.00, Steam build 26317560

Mining Helmet, Bank Refresh and AllStats were tested in-game by the author on 30 October 2026. Speed-only uses the same mount hook as AllStats, restricted to speed; static/host/loader checks passed, but a separate Speed-only gameplay test is pending.
They run through an x64 ASI loader. **DMM and data-archive mounting are not required.**

| Mod | Release | Download |
|---|---|---|
| [Mining Helmet Always On](mods/mining-helmet-always-on/) | 1.3.1 | [Nexus](https://www.nexusmods.com/crimsondesert/mods/3586) |
| [Faster Bank Refresh — Gold and Bonds](mods/faster-bank-refresh-gold-and-bonds/) | 1.2.1 | [Nexus](https://www.nexusmods.com/crimsondesert/mods/3669) |
| [All Mounts LvL 5 All Stats / Speed-only](mods/real-infinite-horse-stamina-with-optional-lvl5-speed-for-all-hor/) | 11 | [Nexus](https://www.nexusmods.com/crimsondesert/mods/2365) |

[Download the ASI release and SHA-256 checksums](https://github.com/gutzufuss1477/Crimson-Desert-Mods/releases/tag/asi-2.04.00-b26317560-r2).
These packages target the later Steam build 26317560 specifically; the version label 2.04.00 alone does not identify the executable.

Close the game, remove older ASIs for the same mod, then copy the new ASI into `Crimson Desert/bin64` beside `CrimsonDesert.exe`. Keep existing INI settings; use the supplied INI only for a new installation. Keep your existing x64 ASI loader, or obtain one from [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader). Do not load both AllStats and Speed.

## Other mods

[Alden shop](mods/alden-all-in-one-progression-shop/), [Healthbar](mods/healthbar-always-on/) and Steelheart Horseshoes use data modifications. This release does not update those downloads or claim compatibility with game 2.04.00. Existing historical source remains available. The obsolete Bank Save Patcher is excluded.

See [build and validation](tooling/README.md), [distribution notes](DISTRIBUTION.md), [security reporting](SECURITY.md) and [license scope](LICENSES.md). Original game data, executables, private diagnostics and credentials are excluded from this repository.
