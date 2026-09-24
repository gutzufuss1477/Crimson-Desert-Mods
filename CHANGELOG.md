# Changelog

## 2.03.02

### All Mounts LvL 5 All Stats / Speed

- Ported both runtime ASI plugins to Crimson Desert 2.03.02 / Steam build 25474236.
- Rebased the verified mount-stat hook from RVA `0x00E68F89` to `0x00E68F09`.
- Retained the existing StatIndex, SubIndex, ordinal and value-range checks.
- Added a second strict signature check at the hook target; mismatching builds fail closed.
- Packaged new DMM-ready releases. In-game validation is still required before marking the port as fully confirmed.

## 2.03.00

### General

- Added **Mining Helmet Always On 1.0.0** for Crimson Desert 2.03.00.
- The mod automatically activates the native blue mining-resource highlight while any headgear is equipped.
- The green full-screen Mining Helmet filter remains disabled.
- The original Mining Helmet and its `B`-key behavior remain available while the physical helmet is equipped.
- The release uses the game's normal visibility and fade distance and contains a strict executable hash guard.
- Updated **All Mounts LvL 5 All Stats** and **All Mounts LvL 5 Speed** for Crimson Desert 2.03.00.
- Reworked the mount stat modification system for compatibility with the current game version.
- Changed the two mount mods from static CharacterInfo data patches to **DMM-managed ASI plugins**.
- The previous static CharacterInfo values can still be changed in 2.03.00, but are no longer reliably used as the active stats of existing Level 5 mounts.
- The new implementation uses the current runtime mount-stat path.
- No Cheat Engine or manual runtime setup is required for normal use.
- Both plugins contain a strict 2.03.00 code-signature guard and fail closed on a mismatching game build.
- Only one Mount LvL 5 variant should be enabled at a time.

### All Mounts LvL 5 All Stats

- Sets Movement Speed, Acceleration, Turning / Handling and Jump to Level 5.
- Runtime path validated in-game on an existing Level 5 Rokade.
- Confirmed result on the validation mount: **4 / 4 / 5 / 5 -> 5 / 5 / 5 / 5**.
- The implementation is not hardcoded to Rokade or Character ID 31378; it targets the compatible mount-stat row structure.

### All Mounts LvL 5 Speed

- Uses the same 2.03.00 mount-stat implementation as the All Stats version.
- Changes only Movement Speed to Level 5.
- Acceleration, Turning / Handling and Jump remain unchanged.

## 2.00.00

### General

- Updated all maintained mods for Crimson Desert 2.00.00.
- Rebuilt the complete mod set specifically for DMM.
- JMM is no longer supported or maintained.
- Removed test/release-candidate suffixes from final package and internal mod names.
- Added reproducible 2.00.00 recipes and validation for the final working implementations.

### Alden AIO Shop + All Items 1 Copper

- Ported the exact original 379-item catalog.
- Preserved original item order and per-item stock quantities.
- Stock distribution remains 258 entries with 1, 93 entries with 99, and 28 entries with 999.
- Purchase prices are set to 1 Copper through DMM semantic ItemInfo intents.
- Uses a validated 2.00.00 StoreInfo replacement for Alden only.

### All Mounts LvL 5 Speed

- Updated all target records for 2.00.00.
- Changes only movement speed to level 5.
- 1,248 validated patches across the compatible mount stat blocks.

### All Mounts LvL 5 All Stats

- Updated all target records for 2.00.00.
- Sets movement speed, acceleration, turning and jump to level 5.
- 4,990 validated patches across the compatible mount stat blocks.

### Steelheart Horseshoes +20 Stamina Regen

- Reworked as a DMM semantic ItemInfo patch.
- Targets only `HorseShoe_HorseArmor_Shoe_III` / item 1000594.
- Sets `enchant_data_list[0].equip_buffs[0].level` from 4 to 20.

### Healthbar - all three variants

- Rebased UI files on the 2.00.00 vanilla files.
- Rebuilt skill 1201 from the ShowHPUI skill while retaining the correct 1201 identity/self-reference.
- Uses only three confirmed CharacterInfo range-helper redirects: Kliff, Kliff_AI and Yann.
- Removed the incorrect broad CharacterInfo skill-reference patching used during early testing.
