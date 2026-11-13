**Faster Bank Refresh - Gold and Bonds 1.4.0**
Updated for Steam build 26636972 (EXE 1.0.0.3663), 13 November 2026. DMM 5.0.0 was used for the current setup. The author reports the bank working in game on this build.

[Support my mods on Ko-fi](https://ko-fi.com/gutzufuss) — optional support helps me keep making updates.

**Features and settings**
Gold and active bond timers default to 15 in-game minutes. [BankRefresh] and [Bonds] each have Enabled=1 and IntervalGameMinutes=15. Gold accepts 1–4320 and bonds 1–10080. Whole numbers only; 60 means one game hour. Restart after INI changes. An older INI without [Bonds] leaves bonds unchanged.
Investments and reinvestments remain manual. Native payouts, risk and unlocks are unchanged. Native checks may briefly delay a refresh after zero; shorter existing countdowns are not extended. No save patcher is required.

**Installation**
Close the game. Remove the previous version in DMM and any duplicate manually installed copy. Back up custom INI settings. Import the complete ZIP through DMM ASI Mods, enable the plugin, mount and restart. Keep the working x64 ASI loader and DmmRuntime.asi. Included INIs are imported and deployed with the ASI. Manual alternative: put the ASI and its INI, if supplied, beside CrimsonDesert.exe in bin64 with an x64 ASI loader. No loader is bundled. Do not combine manual and manager copies.
Do not combine this with old BR04, Bonds Test or other Bank Refresh copies.

**What changed / future updates**
Version 1.4.0 supports the new bank component layout and typed lookup used by this game build. Both timer hook paths passed host and executable validation. The release retains the exact tested ASI and INI.
The native ASI resolves validated code structures and may survive updates that only move compatible code. Changed layouts, changed functions or ambiguous matches can still require an update. Future compatibility is not guaranteed.
Status: %LOCALAPPDATA%\CrimsonDesertMods\Bank_Refresh.status.txt.

**Uninstall**
Close the game, disable/remove the plugin in DMM and remount. Shortened deadlines may be saved; uninstalling does not restore them, so retain a pre-mod save backup.

[Source, release notes and checksums on GitHub](https://github.com/gutzufuss1477/Crimson-Desert-Mods).
**Credits**
Thanks to Jicku12x and c47n1p for requesting bond support, the MinHook contributors and ThirteenAG for Ultimate ASI Loader.


[Current build and validation notes](../../releases/mods-b26636972.md).
