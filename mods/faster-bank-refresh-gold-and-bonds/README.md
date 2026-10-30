# Faster Bank Refresh — Gold and Bonds

## Game 2.04.00 / Steam build 26317560

Current ASI release: **1.2.1**. Tested in-game by the author on 30 October 2026. Native hooks and build/layout checks were updated for this exact Steam build.

Independent gold-bank and bond-investment timers, defaulting to 15 in-game minutes. Preserve your INI. Investments/reinvestment remain manual and native payouts/risk are unchanged. Restart after configuration edits. Shortened saved deadlines are not restored by uninstalling.

[Download on Nexus](https://www.nexusmods.com/crimsondesert/mods/3669) · [GitHub release and checksums](https://github.com/gutzufuss1477/Crimson-Desert-Mods/releases/tag/asi-2.04.00-b26317560)

## Install / update

Close the game. Remove old/test ASIs for the same mod, including duplicate manager-installed copies. Copy the released ASI into `Crimson Desert/bin64` beside `CrimsonDesert.exe`. Keep your existing INI; copy a supplied INI only for a new installation. An [x64 ASI loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) is required and is not included. **DMM and archive mounting are not required.**

To uninstall, close the game and remove this mod's ASI.

## Source and validation

Build: `source/src/build.cmd`. See [build instructions and validation](../../tooling/README.md). `nexus.json` records immutable file IDs and package SHA-256 values. The release ZIPs contain the exact ASI payloads used in the successful gameplay test. Build outputs, game-derived inputs and private diagnostics stay out of Git. Third-party licenses remain with their source.
