# Alden All-In-One Progression Shop

Current releases for **Steam build 26480954**, published 7 November 2026:

- [DLC 17.4.3 — Main File](https://www.nexusmods.com/crimsondesert/mods/2066?tab=files&file_id=20197): requires installed **Charting the Unknown**. 843 offers / 834 distinct items; ten category tabs, including a dedicated DLC tab for all 234 expansion offers. Author confirmed working in game.
- [No DLC 17.3.4 — Optional File](https://www.nexusmods.com/crimsondesert/mods/2066?tab=files&file_id=20198): 609 offers / 600 distinct items; nine categories, no DLC items or requirement. Technical checks passed; no separate gameplay test of this variant.

Both versions add **Long Horn / Langhorn** and **Water / Wasser** to Materials for **1 Copper, 999 merchant stock**. Both are base-game ingredients. Native inventory stack limits remain unchanged. All existing stock/save identities are preserved.

## Categories and assortment

The shared categories contain base-game offers: Abyss 94, Abyss Plans 93, Books 71, Recipes 95, Blueprints 125, Materials 41, Contracts 50 and Original 40, plus All.

The DLC version adds **DLC 234**: 18 Abyss gears, 18 Abyss plans, 40 crafting/knowledge books, 19 recipes, 104 blueprints and 35 materials, including Natural Pearl. Expansion offers appear in DLC and All. All books, including doors/windows/decor knowledge, remain at Alden.

Labels follow all 15 game text languages and size/wrap automatically. Counts are catalog totals; original unlock conditions and sold-out offers can reduce availability. The Original tab retains 40 native offers, stocks and unlock/restock conditions. 31 retain normal prices; nine shared items remain 1 Copper, with separate stock records. Tiered Abyss Gear stays at Tier I; craft higher tiers yourself.

## Install / update

Use **DMM 4.7.4 or later**, its ASI loader and the [Microsoft Visual C++ v14 Redistributable, x64](https://aka.ms/vc14/vc_redist.x64.exe). Close the game and unmount. Completely replace the previous Alden folder; do not merge folders. Import the chosen ZIP, enable its matching 1 Copper component, and replace/enable the included **Alden Category Tabs.asi**. Mount again and restart.

Keep only one variant and matching helper active: DLC package/helper **17.4.3**, or No DLC package/helper **17.3.4**. Do not manually copy the internal `files/` folder into the game. Mount again after installing the DLC or updating DMM. DMM 4.7.3 fixed missing/broken DLC offers at ordinary vendors by adding the vendor to the DLC shop list during mounting.

## Compatibility and validation

Original [Better Inventory and Trade UI 3.3](https://www.nexusmods.com/crimsondesert/mods/452) remains optional and separate. There are no shared replacement paths or required load order. Follow its author's DLC/non-DLC texture instructions. The native 2x4 or Better Inventory 3x5 buy grid is preserved.

Original [Character Creator Enhanced 9.1.32 / Equip All Armor](https://www.nexusmods.com/crimsondesert/mods/837) remains separate. Both orders and repeated application of the price patches with Equip All Armor and Steelheart pass; other shop, price or LocalStringInfo mods can still conflict.

Current supplied DLC data were checked for item IDs, names/descriptions in 15 languages, icon references and all 40 book knowledge/action references. Native/UI regressions, stock preservation and independent price-composition checks pass. Other 439 current merchants are unchanged. No DLC item definitions, textures or models are bundled.

**The author confirmed DLC 17.4.3 working in game on 7 November 2026.** Final ASI and all game payloads match that build exactly; only the readme was finalized afterward. No DLC 17.3.4 is technically validated but has not had a separate gameplay test. The previous regular 17.3.3 was confirmed working. The earlier separate ASI crash report has not been reproduced or individually resolved.

Final packages/helpers passed a local Defender scan. Published downloads were retrieved and matched exactly. Runtime fixtures and broad author confirmation do not cover every item or mod combination. For problems, include DMM/game versions, active ASIs, `Alden Category Tabs.asi.log` and the DMM mount log.

## Special Thanks

Special thanks to the two community members who made the DLC available to me, allowing me to build and test this update. Your support is greatly appreciated!

## Uninstall

Close the game, unmount, disable the shop, its price component and category ASI, then remount. Purchased items and money spent are not rolled back.

[Changelog](CHANGELOG.md) · [Release notes and checksums](../../releases/alden-current-dlc-b26480954.md) · [Validation recipe](../../recipes/alden-current-dlc-b26480954/) · [Build source](source/)

Full game-data packages remain Nexus-only. Git contains source, catalog/localization recipes, hashes and validation records.
