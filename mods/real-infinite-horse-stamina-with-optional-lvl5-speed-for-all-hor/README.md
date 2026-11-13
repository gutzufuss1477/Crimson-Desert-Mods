**Mount Level 5 Display v15 / Steelheart Horseshoes v10**
Updated for Steam build 26636972 (EXE 1.0.0.3663), 13 November 2026. DMM 5.0.0 was used for the current setup.
**Scope correction:** All Stats and Speed Only change the mount stat display/cache. A corresponding increase to actual movement speed, acceleration, turning, jumping or stamina has not been established. The earlier description promised more than the implementation demonstrates. Please choose these ASIs for the level-5 display; Steelheart is the separate equipped stamina-regeneration modification.

[Support my mods on Ko-fi](https://ko-fi.com/gutzufuss) — optional support helps me keep making updates.

**Choose your files**

- **All Stats v15 (ASI):** sets the four supported mount stat gauges to level 5. The author confirmed the displayed values on the current build.
- **Speed Only v15 (ASI):** sets only the speed gauge to level 5. The other gauges are unchanged. Current automated checks passed; no separate gameplay test on this build.
- **Steelheart Horseshoes v10 (DMM):** changes the equipped horseshoes' stamina-regeneration buff from level 4 to 20. Equip the item. No ASI needed. This existing download is unchanged.

Use All Stats OR Speed Only, never both. Steelheart is optional and can be used separately.

**Install a mount ASI**
Close the game. Remove the previous version in DMM and any duplicate manually installed copy. Back up custom INI settings. Import the complete ZIP through DMM ASI Mods, enable the plugin, mount and restart. Keep the working x64 ASI loader and DmmRuntime.asi. Included INIs are imported and deployed with the ASI. Manual alternative: put the ASI and its INI, if supplied, beside CrimsonDesert.exe in bin64 with an x64 ASI loader. No loader is bundled. Do not combine manual and manager copies.
The two mount ZIPs do not need an INI. DMM is optional for these ASIs.

**v15 fixes and validation**
Updated the current mount UI field mapping and corrected native changed-value notifications. Each variant passed 1,354 host checks, including captured current fields, unrelated UI rejection and native notification continuation. A hook being installed is now distinguished from matching fields actually being applied. The published ASIs are the tested candidates without binary changes.
The native ASI resolves validated code structures and may survive updates that only move compatible code. Changed layouts, changed functions or ambiguous matches can still require an update. Future compatibility is not guaranteed.
Status: %LOCALAPPDATA%\CrimsonDesertMods\Mount_AllStats.status.txt or Mount_Speed.status.txt.

**Steelheart installation and compatibility**
Close the game and unmount. Import the complete Steelheart v10 ZIP through DMM, enable Steelheart Horseshoes - 20 Stamina Regen, mount and restart. Equip the horseshoes and keep only one Steelheart patch active.
Its item-ID/equip-buff field patch preserves unrelated item edits, including Equip All Armor. Compatibility was confirmed on the earlier build; the unchanged patch passed current data checks, with no separate new gameplay confirmation. Changes to the same equip buff can still conflict. This data patch is not update-independent.

**Uninstall**
Close the game, disable/remove the chosen mod in DMM and remount the remaining mods. Remove a manual ASI manually if applicable.

[Source, release notes and checksums on GitHub](https://github.com/gutzufuss1477/Crimson-Desert-Mods).
**Credits**
Thanks to the Crimson Desert modding community, ThirteenAG for Ultimate ASI Loader and everyone who tested the mods.


[Current build and validation notes](../../releases/mods-b26636972.md).
