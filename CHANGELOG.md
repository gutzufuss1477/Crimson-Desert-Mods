# Changelog

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
