**Healthbar Always On v5**
Updated for Steam build 26636972 (EXE 1.0.0.3663), 13 November 2026. DMM 5.0.0 was used for the current setup. See enemy health bars without a Health Detection Device or special helmet.

[Support my mods on Ko-fi](https://ko-fi.com/gutzufuss) — optional support helps me keep making updates.

**Choose ONE of three distinct variants**

- **Vanilla Multitarget:** classic red bars with a dark frame; multiple enemies. ASI helper included.
- **Classic Vanilla Single Target:** the same framed style; one target. No Healthbar ASI required.
- **Caites Multitarget:** compact thin red bars without the thick frame; multiple enemies. ASI helper included.

Multitarget keeps bars when switching targets and retains native removal when enemies die.

**Installation — one ZIP through DMM**

- Close the game, unmount and remove the previous Healthbar variant in DMM.
- Import the complete ZIP for your chosen variant. No manual game-folder copying is required.
- Enable the Healthbar data mod. For either Multitarget style, also enable the included Healthbar Multitarget under ASI Mods; DMM may leave its checkbox off after import.
- Keep the existing ASI loader and DmmRuntime.asi, mount and restart.

Use only one data variant and one helper copy. Disable/remove the Multitarget helper when selecting Classic. Remove old manual NativeFix/Healthbar ASIs when migrating to DMM. To uninstall, disable/remove the data and helper, then remount.

**What changed / validation**
Rebased the skill and UI payloads onto the current game build and updated native resolution. All three visual styles are retained. Of 2,160 skill records, only the intended skill 1201 changes; the other 2,159 are preserved.
Vanilla Multitarget was confirmed in game on this build. Classic and Caites passed data/style/native checks but have no separate current-build gameplay confirmation. Other characters were not separately tested. All published functional payloads are identical to the validated candidates.

**Future updates / conflicts**
The native ASI resolves validated code structures and may survive updates that only move compatible code. Changed layouts, changed functions or ambiguous matches can still require an update. Future compatibility is not guaranteed.
This applies to the native Multitarget helper only. The full mod still includes game-data/UI files for build 26636972 and can require rebuilding after a patch. Classic is data-only. Other changes to the same skill or subtitletagview.html/css can conflict.

**Credits**
Blablup / gutzufuss1477. Thanks to [Caites](https://www.nexusmods.com/profile/Caites) for [Persistent HP Bars](https://www.nexusmods.com/crimsondesert/mods/2409), the original multitarget work/inspiration and compact presentation, and to the community testers.
[Source, release notes and checksums on GitHub](https://github.com/gutzufuss1477/Crimson-Desert-Mods).


[Current build and validation notes](../../releases/mods-b26636972.md).
