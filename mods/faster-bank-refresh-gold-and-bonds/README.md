# Faster Bank Refresh — Gold and Bonds

**Bank Refresh 1.3.1** restores the gold-bank and active bond timers on **Crimson Desert 2.04.06 (Steam build 26423880)**. The author confirmed the mod working in game on 5 November 2026.

Independent timers default to 15 in-game minutes. Investments/reinvestment remain manual; native payouts and risk are unchanged. Normal bank checks may briefly delay refresh after zero. Shortened saved deadlines are not restored by uninstalling.

[Nexus](https://www.nexusmods.com/crimsondesert/mods/3669?tab=files&file_id=19612) · [GitHub release](https://github.com/gutzufuss1477/Crimson-Desert-Mods/releases/tag/bank-v1.3.1-b26423880)

## Install

Close the game, remove previous versions/manual duplicates, import the complete ZIP in DMM under ASI Mods, enable and mount. DMM imports the included INI too; preserve existing settings. The current setup uses DMM 4.7.2. Keep the existing ASI loader and DmmRuntime.asi.

Manual alternative: put Bank_Refresh.asi and Bank_Refresh.ini beside CrimsonDesert.exe in bin64 with an [x64 ASI loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader). DMM is optional. Restart after INI edits. Close the game before disabling/removing the plugin.

## Compatibility

The ASI now finds validated code structures instead of relying on fixed addresses or one executable hash. It may keep working after game updates that only move code. Changed code/layouts or ambiguous matches are refused before hooks are installed. Future compatibility is not guaranteed.

Game 2.04.06 changed a field access in the gold refresh routine. The ASI recognizes both audited layouts and checks the calls to both timer routines before activating. Startup status: %LOCALAPPDATA%/CrimsonDesertMods/Bank_Refresh.status.txt.

Build: source/src/build.cmd. Validation includes 24,565 core checks, 284 hook checks, 33 initialization cases, two production loads and executable checks on builds 26423880 and 26361232. An unrecognized gold member offset and the older 26329202 layout both leave the timers inactive. See [tooling](../../tooling/README.md) and nexus.json for hashes/file IDs.
