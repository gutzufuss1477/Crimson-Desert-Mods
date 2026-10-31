# Horse stamina and mount stats

Updated for **Crimson Desert 2.04.02 / Steam build 26329202**.

| Download | Version | Installation | Validation |
|---|---|---|---|
| All Mounts LvL 5 All Stats | 12 | ASI loader | Gameplay confirmed 31 October 2026 |
| All Mounts LvL 5 Speed-only | 12 | ASI loader | Static/host/loader checks passed; separate gameplay test pending |
| Steelheart Horseshoes 20 Stamina Regen | 9 | DMM 4.5.1+ | Mount and gameplay confirmed 31 October 2026 |

AllStats changes mount speed, acceleration, turning and jump. Speed-only changes movement speed alone. **Use AllStats OR Speed-only, never both.** These ASIs do not require Steelheart or DMM.

Steelheart raises the equipped horseshoes' stamina-regeneration buff from level 4 to 20. Equip the item. It can be used alone or with either mount ASI.

[Downloads on Nexus](https://www.nexusmods.com/crimsondesert/mods/2365) · [GitHub release and checksums](https://github.com/gutzufuss1477/Crimson-Desert-Mods/releases/tag/mods-2.04.02-b26329202)

## Install a mount ASI

Close the game. Remove older/test ASIs for the same mod, including duplicate manager-installed copies. Copy the released ASI into `Crimson Desert/bin64` beside `CrimsonDesert.exe`. Preserve existing INI settings; copy a supplied INI only for a new installation. An [x64 ASI loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) is required and is not included. **DMM and data-archive mounting are not required.**

To uninstall, close the game and remove this mod's ASI.

## Install Steelheart

Close the game and unmount in DMM. Import `Steelheart_Horseshoes_20_Stamina_Regen_2.04.02.zip` into DMM 4.5.1 or a compatible newer release. Disable the previous Steelheart version, enable the new package and mount. Keep DMM's installed runtime and ASI loader. Restart the game. To uninstall, close the game, unmount, disable Steelheart and remount the remaining mods.

## Source and validation

Build the ASIs with `tooling/build_allstats_204.cmd` and `tooling/build_speed_204.cmd`. Both passed 1,135 host checks plus native/loader checks. Speed-only shares the AllStats hook with a narrower stat filter. Steelheart uses the current item table and changes only the relevant equip-buff. See [build instructions](../../tooling/README.md) and `nexus.json` for exact IDs/hashes. Older downloads remain archived for their labelled game versions.
