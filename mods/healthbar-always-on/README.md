# Healthbar Always On

**Release 4**, for **Steam build 26361232**.

| Variant | Appearance | Targets | Components |
|---|---|---|---|
| Vanilla Multitarget | Framed red bars | Multiple | DMM data + bundled ASI |
| Classic Vanilla Single Target | Framed red bars | One | DMM data only |
| Caites Multitarget | Compact thin red bars | Multiple | DMM data + bundled ASI |

[Complete Nexus downloads](https://www.nexusmods.com/crimsondesert/mods/2406?tab=files) · [Source/helper release](https://github.com/gutzufuss1477/Crimson-Desert-Mods/releases/tag/mods-b26361232)

## Install

Close the game and unmount/remove the old Healthbar variant. Import **one complete ZIP** in DMM. Enable the data mod and, for Multitarget, the included **Healthbar Multitarget** under ASI Mods. Keep DMM's ASI loader, mount and restart. No separate helper download or manual file copy is needed. DMM 4.6.4 import/deployment was verified.

Remove old manual NativeFix/Healthbar helpers before migrating. Never enable multiple variants or duplicate helpers. The helper selects the style automatically and remains inactive with Classic. Close the game before changing or uninstalling variants; remount the remaining mods afterwards.

## Updates and validation

The native helper gains structural code detection: The ASI now finds validated code structures instead of relying on fixed addresses or one executable hash. It may keep working after game updates that only move code. Changed code/layouts or ambiguous matches are refused before hooks are installed. Future compatibility is not guaranteed.

**The DMM data/UI packages remain specific to build 26361232.** Classic is data-only and does not gain ASI-based update resilience. Conflicting changes to the same skill or subtitletagview HTML/CSS can prevent correct operation.

The skill overlay was rebuilt from current vanilla inputs, preserving the new ability ID and all unrelated records. Three previous visual styles remain unchanged. Vanilla Multitarget was confirmed working in game on 2 November 2026. Classic and Caites passed current data/style/host checks, but were not separately replaytested on this build. Other characters were not separately tested.

Released functional payloads are unchanged from the validated candidates. Native patch guards, atomicity, Classic isolation and native target cleanup remain tested. Build the helper with source/native/build.cmd; see [tooling](../../tooling/README.md) for data recipes and packaging. Full game data/UI assets remain Nexus-only.

Credits: Blablup / gutzufuss1477; [Caites — Persistent HP Bars](https://www.nexusmods.com/crimsondesert/mods/2409). The helper's MIT license covers only original helper code.
