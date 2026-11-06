# Alden 17.3.3 / DLC 17.4.2 — Steam 26480954

Published on 6 November 2026. These hotfixes update the existing releases for Steam build **26480954**. The previous category helpers check the executable hash and refuse this newer build.

| Package | Nexus file | Offers | ZIP bytes |
|---|---|---:|---:|
| Regular 17.3.3 | [20056 — main download](https://www.nexusmods.com/crimsondesert/mods/2066?tab=files&file_id=20056) | 607 / 598 distinct items | 4,365,411 |
| DLC 17.4.2 | [20057 — optional download](https://www.nexusmods.com/crimsondesert/mods/2066?tab=files&file_id=20057) | 841 / 832 distinct items | 4,372,416 |

The optional file requires **Charting the Unknown**. Install only one variant. Completely replace the previous data package and its matching **Alden Category Tabs.asi**; keep the matching 1 Copper component enabled. Old versions 17.3.2 and 17.4.1 are archived on Nexus.

## Changes and retained behavior

- Update native addresses, executable/function guards and native regression fixtures for the new executable.
- Rebase official localization onto the current game data, preserving the same nine tab labels in all 15 languages and all 439 other merchants.
- Retain catalogs, prices, stock/save identities, filtering and single-step keyboard/controller behavior. The Original tab retains its 40 native offers; nine shared items still cost 1 Copper.
- Retain all 234 DLC offers, including Natural Pearl and 40 DLC crafting books. Books remain at Alden; no Alfred changes are included.
- Preserve vanilla and original Better Inventory 3.3 layouts without shared replacement paths or an Alden/Better Inventory load-order requirement. Follow Better Inventory's own DLC texture instructions.

## Validation and limits

Both builds passed native category and runtime UI regression tests, the wrong-process guard, catalog/localization checks and price composition with original Equip All Armor and Steelheart in both application orders. All 706 regular / 958 DLC price intents are unchanged. Both ZIPs and ASIs passed direct local Microsoft Defender scans. The published Nexus downloads were retrieved and matched the package hashes below.

**These new builds have no gameplay confirmation yet.** The author's previous regular-version confirmation applies to 17.3.2 on Steam 26448710. The author does not own the DLC; the optional catalog uses available DLC 2.04.05 reference data, while current DLC tables and full runtime behavior remain unverified. DLC owners are asked to test. The separate reported ASI crash has not been reproduced, and this is not a confirmed fix for that report.

No old DLC definitions, textures, models or item localization are bundled. Helpers remain unsigned; a clean local Defender scan is not a guarantee about other engines or future signatures.

## SHA-256

| Artifact | SHA-256 |
|---|---|
| Regular `Alden_Shop.zip` | `24c8d5b5cbe4848b3985b1750ff7ea301b761b0a3886b07233db67b083ec0351` |
| Regular `Alden Category Tabs.asi` | `33dafb13c25815f7ded5b3b8a577151c6276a90e8e1c817a79e0192a3b5968cc` |
| DLC `Alden_Shop_DLC.zip` | `08f0ae50f8fc6c85bebd09a30c16b44a23aea97a3d3f64af3c35ba9effd8fce7` |
| DLC `Alden Category Tabs.asi` | `0d90ac13cf72efeb22440cd8ce1307ee193e596d12699c23ced19a0d59b30d2c` |
| Supported `CrimsonDesert.exe` | `c94ad6c3ea54970a6d2da7a2719753210a7728a91698ed3405dd9125470ed98f` |

[Source and build instructions](../mods/alden-all-in-one-progression-shop/source/) · [Validation and publication records](../recipes/alden-hotfix-b26480954/)
