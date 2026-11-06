# Alden All-In-One Progression Shop

Current releases: **17.3.3 regular** and **17.4.2 optional DLC**, for **Steam build 26480954**. The previous helpers support build 26448710 and refuse the new executable. Replace both the data package and its matching category ASI.

- [Regular download 17.3.3](https://www.nexusmods.com/crimsondesert/mods/2066?tab=files&file_id=20056): 567 mod offers at 1 Copper plus 40 original offers; 607 offers / 598 distinct items. No DLC-exclusive items.
- [Optional DLC download 17.4.2](https://www.nexusmods.com/crimsondesert/mods/2066?tab=files&file_id=20057): requires installed **Charting the Unknown**. 841 offers / 832 distinct items, including all 234 previous DLC offers, Natural Pearl and 40 DLC crafting books at Alden.

The hotfix changes executable compatibility and refreshes official localization. Catalogs, prices, stock/save identities, category filtering and keyboard/controller behavior are unchanged. No Alfred changes are included.

Nine tabs follow all 15 game text languages and size/wrap automatically: All, Abyss, Abyss Plans, Books, Recipes, Blueprints, Materials, Contracts and Original. Counts show catalog totals; unlock conditions and sold-out offers can reduce availability. Tiered Abyss Gear stays at Tier I; craft higher tiers yourself.

The Original tab preserves Alden's 40 native offers, stocks and unlock/restock conditions. 31 retain normal prices; nine shared items remain 1 Copper in both categories, with separate stock records. Existing stock/save identities are retained; historical vanilla purchase counters are not migrated.

## Install / update

Use DMM with support for this game build and its ASI loader, plus the [Microsoft Visual C++ v14 Redistributable, x64](https://aka.ms/vc14/vc_redist.x64.exe). Close the game and unmount. Completely replace the previous Alden package; do not merge folders. Import `Alden_Shop.zip` or `Alden_Shop_DLC.zip`, enable its matching 1 Copper component and replace/enable the included `Alden Category Tabs.asi`. Mount and restart.

Keep only one Alden variant, price patch and helper active. Both regular components must be 17.3.3; both DLC components must be 17.4.2. Do not manually copy the internal `files/` folder into the game.

## Compatibility and validation

Original [Better Inventory and Trade UI 3.3](https://www.nexusmods.com/crimsondesert/mods/452) remains optional and separate. No shared replacement paths or required load order. Follow its author's DLC/non-DLC texture instructions. The original 2x4 or Better Inventory 3x5 buy grid is preserved.

Original [Character Creator Enhanced 9.1.32 / Equip All Armor](https://www.nexusmods.com/crimsondesert/mods/837) stays separate and unmodified. The unchanged 706 regular / 958 DLC price intents preserve its armor edits. Composition with Equip All Armor and Steelheart passed in both orders and on repeat application. Other shop, price or LocalStringInfo mods can still conflict.

Both hotfixes passed native/UI/data checks and final local Defender scans. Published ZIP downloads were retrieved and their SHA-256 verified. **These builds have not yet been confirmed in game.** The author's previous normal-version confirmation applies to 17.3.2 on build 26448710.

**The author does not own the DLC. DLC owners are asked to test the optional file.** Its catalog uses the available DLC 2.04.05 reference; current DLC tables and full gameplay remain unverified. No old DLC definitions or assets are bundled. The separate reported ASI crash has not been reproduced; this hotfix is not a confirmed fix for that report.

The shared Microsoft runtime and Control Flow Guard remain in use. Helpers are unsigned; other antivirus engines and future signatures are unverified. Protection settings were not changed.

[Changelog](CHANGELOG.md) · [Release notes and checksums](../../releases/alden-hotfix-b26480954.md) · [Validation recipe](../../recipes/alden-hotfix-b26480954/) · [Build source](source/)

## Uninstall

Close the game, unmount, disable the shop, its price component and category ASI, then remount. Purchased items and money spent are not rolled back.

Full game-data packages remain Nexus-only. Git contains source, catalog/localization recipes, hashes and validation records.
