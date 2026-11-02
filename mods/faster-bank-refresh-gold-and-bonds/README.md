# Faster Bank Refresh — Gold and Bonds

**Release 1.3.0**, baseline **Steam build 26361232**. Gold and active bond timers confirmed working in game on 2 November 2026. The released ASI is unchanged from the tested candidate.

Independent timers default to 15 in-game minutes. Investments/reinvestment remain manual; native payouts and risk are unchanged. Normal bank checks may briefly delay refresh after zero. Shortened saved deadlines are not restored by uninstalling.

[Nexus](https://www.nexusmods.com/crimsondesert/mods/3669) · [GitHub release](https://github.com/gutzufuss1477/Crimson-Desert-Mods/releases/tag/mods-b26361232)

## Install

Close the game, remove previous versions/manual duplicates, import the complete ZIP in DMM under ASI Mods, enable and mount. DMM imports the included INI too; preserve existing settings. Import/deployment was verified with DMM 4.6.4. Keep the existing ASI loader and DmmRuntime.asi.

Manual alternative: put Bank_Refresh.asi and Bank_Refresh.ini beside CrimsonDesert.exe in bin64 with an [x64 ASI loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader). DMM is optional. Restart after INI edits. Close the game before disabling/removing the plugin.

## Compatibility

The ASI now finds validated code structures instead of relying on fixed addresses or one executable hash. It may keep working after game updates that only move code. Changed code/layouts or ambiguous matches are refused before hooks are installed. Future compatibility is not guaranteed.

Bank's records/count fields were updated for the new object layout. Startup status: %LOCALAPPDATA%/CrimsonDesertMods/Bank_Refresh.status.txt.

Build: source/src/build.cmd. Validation includes 24,565 core checks, 284 hook checks, 33 initialization cases, two production loads and current/previous executable resolver checks; the old bank layout is deliberately refused. See [tooling](../../tooling/README.md) and nexus.json for hashes/file IDs.
