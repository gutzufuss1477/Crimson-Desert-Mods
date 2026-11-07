# Alden release — current DLC / Steam 26480954

Published on 7 November 2026. **DLC gameplay confirmed by the author; No DLC separately validated offline.** The author supplied a current DLC installation on C:. The EXE is unchanged from the prior hotfix; all 234 previously selected DLC keys still resolve. ItemInfo changed in several stack/inventory-related fields. The new packages preserve installed DLC definitions instead of distributing older replacements.

| Variant | Nexus role | Offers | Categories | Price intents |
|---|---|---:|---:|---:|
| 17.4.3 DLC | Main | 843 / 834 distinct items | 10, including DLC (234) | 960 |
| 17.3.4 No DLC | Optional | 609 / 600 distinct items | 9 | 708 |

Both append Long Horn (key 756518 / Deer_Antler / Langhorn) and Water (22008 / Water / Wasser) to Materials at 1 Copper with 999 stock. Both are present in the current **base** ItemInfo. Inventory stack limits stay native. The new stock/save indices are 841–842 for DLC and 607–608 for No DLC. All previous stock rows are byte-identical; other 439 current merchants are unchanged. The DLC rows at indices 607–840 are now shown exclusively in DLC and All. Forty existing knowledge books, including decor, remain at Alden.

Use [build_checked_shop.py](../../mods/alden-all-in-one-progression-shop/source/build_checked_shop.py) and [test_checked_shop.py](../../mods/alden-all-in-one-progression-shop/source/test_checked_shop.py), with the commands in the [source instructions](../../mods/alden-all-in-one-progression-shop/source/). The builder pins previous published archives and the exact supported EXE. It needs the matching previous generated native tree, current data, parser and MSVC toolchain. This repository alone is not a complete set of build inputs.

The validator's current-data folder contains read-only extracted base/DLC tables and 15-language item/local-string files under `raw/{base,dlc}/{archive-group}/{original-path}`, plus `inventory.json` entries with `kind`, `group`, `path` for indexed textures. `input-hashes.json` records the actual extracted sources. Game files, native fixtures and complete data payloads are kept outside Git.

## Checks and limits

- Native fixtures cover all category counts, purchase-row identities, Original overlaps, invalid-row atomic refusal, single-step keyboard/controller settings and supported vanilla/Better Inventory UI insertion. The runtime test host executes the native resource/dependency path with simulated final rendering/CSS compilation; it is not a running game.
- Current-data reports check every offered item's names/descriptions in 15 languages, icon asset paths, all 40 DLC book use/knowledge references, exact old stock preservation and untouched merchants.
- Price composition uses original Equip All Armor and Steelheart in both orders and on repeat application. All parsed fields are compared to independently constructed expected records. Base/DLC price subsets are also applied separately. This is not a full DMM mount simulation.
- Original Better Inventory 3.3 has no shared file-replacement path with either package. No third-party mod is modified or bundled.
- `defender.json` records the final staged folder scan and artifact hashes. No security settings were changed.

## DMM requirement and investigation

Require **DMM 4.7.4+**, then mount again with the DLC installed. The installed 4.7.4 executable's embedded changelog states that 4.7.3 fixed broken/missing DLC offers at ordinary vendors by adding the vendor to the DLC shop list during mounting. This matches the reported missing-icon/price symptom, but the reporter's exact DMM version is unknown. The separate reported ASI crash is not reproduced and is not claimed fixed.

The author confirmed the DLC version working in game on 7 November. This broad confirmation does not individually certify every item or every combination of mods. Final ASI and game payloads are identical to the confirmed build; only the release readme changed. No DLC 17.3.4 has not had a separate gameplay test. Reports should include DMM/game versions and both logs.

`nexus-dlc-description.txt` and `nexus-no-dlc-description.txt` contain published file descriptions. `*-published-nexus.json` records file IDs 20197 (DLC main) and 20198 (No DLC optional), archived predecessors and exact downloaded-byte verification. `nexus-description.bbcode.txt`, `nexus-page-verification.json` and `nexus-changelogs.json` record the updated page and anonymous Special Thanks.

[Release notes and checksums](../../releases/alden-current-dlc-b26480954.md).
