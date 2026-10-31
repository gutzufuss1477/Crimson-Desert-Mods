# Mining Helmet Always On

Current release: **1.3.2**, Crimson Desert **2.04.02 / Steam build 26329202**. Confirmed working in-game by the author on 31 October 2026. Updated native addresses and exact-build checks; the released ASI retains the tested bytes.

Highlights ore with any headgear. Range 10–120 (default 60), GlowStrength 0.5–1.0 (default 0.5), with live INI reload. Fast travel and character switching remain available. Remove the old separate Mining_Ore_Group_Range test ASI. Status: `%LOCALAPPDATA%/MiningHelmetAlwaysOn/MH132/latest-status.txt`.

[Download on Nexus](https://www.nexusmods.com/crimsondesert/mods/3586) · [GitHub release and checksums](https://github.com/gutzufuss1477/Crimson-Desert-Mods/releases/tag/mods-2.04.02-b26329202)

## Install / update

Close the game. Remove older/test ASIs for the same mod, including duplicate manager-installed copies. Copy the released ASI into `Crimson Desert/bin64` beside `CrimsonDesert.exe`. Preserve existing INI settings; copy a supplied INI only for a new installation. An [x64 ASI loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) is required and is not included. **DMM and data-archive mounting are not required.**

To uninstall, close the game and remove this mod's ASI.

## Source and validation

Build: `source/src/build.cmd`. See [build instructions](../../tooling/README.md). `nexus.json` records the published file IDs and package hashes. Build outputs, game inputs and private diagnostics stay out of Git. Third-party licenses remain with their source.
