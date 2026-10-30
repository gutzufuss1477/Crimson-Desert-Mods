# Horse stamina and mount stats

## Game 2.04.00 / Steam build 26317560

Current ASI release: **11**. Tested in-game by the author on 30 October 2026. Native hooks and build/layout checks were updated for this exact Steam build.

All Mounts LvL 5 All Stats changes mount speed, acceleration, turning and jump. Steelheart is not required. Use only one mount variant; never load AllStats and Speed together.

[Download on Nexus](https://www.nexusmods.com/crimsondesert/mods/2365) · [GitHub release and checksums](https://github.com/gutzufuss1477/Crimson-Desert-Mods/releases/tag/asi-2.04.00-b26317560)

## Install / update

Close the game. Remove old/test ASIs for the same mod, including duplicate manager-installed copies. Copy the released ASI into `Crimson Desert/bin64` beside `CrimsonDesert.exe`. Keep your existing INI; copy a supplied INI only for a new installation. An [x64 ASI loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) is required and is not included. **DMM and archive mounting are not required.**

To uninstall, close the game and remove this mod's ASI.

## Older variants

Steelheart Horseshoes v8 targets game 2.01.00 and is a separate data mod. Speed-only v10 targets game 2.03.02. Those downloads remain historical alternatives for their labelled versions; this release does not update or confirm them for game 2.04.00.

## Source and validation

Build from repository root: `tooling/build_allstats_204.cmd`. See [build instructions and validation](../../tooling/README.md). `nexus.json` records immutable file IDs and package SHA-256 values. The release ZIPs contain the exact ASI payloads used in the successful gameplay test. Build outputs, game-derived inputs and private diagnostics stay out of Git. Third-party licenses remain with their source.
