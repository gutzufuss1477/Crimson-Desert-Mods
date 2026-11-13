**Mining Helmet Always On 1.5.0**
Updated for Steam build 26636972 (EXE 1.0.0.3663), 13 November 2026. DMM 5.0.0 was used for the current setup. The author confirmed ore highlighting working in game on this build.
Highlights nearby ore without requiring the mining helmet. Keep your preferred headgear.

[Support my mods on Ko-fi](https://ko-fi.com/gutzufuss) — optional support helps me keep making updates.

**Features and configuration**
Range=60 and GlowStrength=0.5 are the defaults. Range accepts 10–120 game world units; GlowStrength accepts 0.5–1.0. Edit Mining_Helmet_Always_On.ini beside the ASI and save while playing. Use decimal points; invalid edits preserve the last valid settings. Visibility, line of sight and distance fading still affect the glow.
Includes live INI reload, ore re-entry handling and native material handoff. Fast travel, character switching and the quick wheel remain available. No activation hotkey, helmet capture, data overlay or separate range plugin is required.

**Installation**
Close the game. Remove the previous version in DMM and any duplicate manually installed copy. Back up custom INI settings. Import the complete ZIP through DMM ASI Mods, enable the plugin, mount and restart. Keep the working x64 ASI loader and DmmRuntime.asi. Included INIs are imported and deployed with the ASI. Manual alternative: put the ASI and its INI, if supplied, beside CrimsonDesert.exe in bin64 with an x64 ASI loader. No loader is bundled. Do not combine manual and manager copies.
Remove old Mining_Ore_Group_Range test plugins too. Keep only one Mining Helmet ASI.

**What changed / compatibility**
Version 1.5.0 adapts the typed lookup and material-manager path to the new game build. Host, resolver and runtime checks passed; the native file is identical to the author-tested candidate.
The native ASI resolves validated code structures and may survive updates that only move compatible code. Changed layouts, changed functions or ambiguous matches can still require an update. Future compatibility is not guaranteed.
The actual helmet/lantern takes control while active; automatic glow resumes after a short delay. Pressing B with the helmet still invokes its separate native mode. Other mods changing the same detection materials, handlers or ore group can conflict.
Status files are under %LOCALAPPDATA%\MiningHelmetAlwaysOn. Include the latest status and your Steam build when reporting a refusal. Disable/remove the plugin in DMM to uninstall.

[Source, release notes and checksums on GitHub](https://github.com/gutzufuss1477/Crimson-Desert-Mods).
**Credits**
Thanks to Tsuda Kageyu and the MinHook contributors, ThirteenAG for Ultimate ASI Loader and the testers. MinHook's license is included; the loader is separate.


[Current build and validation notes](../../releases/mods-b26636972.md).
