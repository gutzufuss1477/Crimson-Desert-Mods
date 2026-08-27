# Validation report - Crimson Desert 2.00.00

## Final package set

Seven final packages are stored in `release-assets/2.00.00/` with clean `*_2.00.00` names and no DMM/RC/test suffixes.

## Builder reproduction

The current builder was run against the clean 2.00.00 vanilla baseline including BuffInfo. For all seven mods:

- generated manifests match the final confirmed package manifests exactly;
- all generated game-data overlays match the final package overlays byte-for-byte;
- package roots and internal mod names match the final ZIP filenames;
- automated repository tests pass.

## In-game evidence from the porting session

- Alden AIO Shop: confirmed after restoring the exact original 379-item catalog, original order and original stock quantities.
- Steelheart Horseshoes: confirmed after changing to the semantic ItemInfo equip-buff level patch.
- All Mounts LvL 5 All Stats: confirmed on the tested horse.
- Healthbar Vanilla Multitarget: confirmed in game.
- All Mounts LvL 5 Speed: not separately tested in game; its 1,248 patches are the exact movement-speed subset of the confirmed 4,990-patch All-Stats version.
- Caites Multitarget and Classic Vanilla Single Target: not separately re-tested after final cleanup; they share the same confirmed skill overlay and three CharacterInfo redirects and differ only in the UI variant.

## Important conflicts

- Enable only one mount variant at a time.
- Enable only one Healthbar variant at a time.
