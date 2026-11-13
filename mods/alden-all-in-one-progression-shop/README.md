**Alden Shop DLC 17.4.6 / No DLC 17.3.7**
Updated for Steam build 26636972 (EXE 1.0.0.3663), 13 November 2026. DMM 5.0.0 was used for the current setup. Fixes the startup UI problem and category switching after this game update. The DLC shop and category tabs are author-confirmed working in game.

[Support my mods on Ko-fi](https://ko-fi.com/gutzufuss) — optional support helps me keep making updates.

**Choose one complete package**

- **DLC 17.4.6 — Main File:** requires installed Charting the Unknown. 840 authored offers / 831 distinct items, including 234 DLC offers and the 40 original offers. Nine category tabs plus All.
- **No DLC 17.3.7 — Optional File:** 606 offers / 597 distinct items, including 40 original offers. Eight category tabs plus All. No DLC items or requirement.

Keep one Alden package, its matching 1 Copper component and one matching Alden Category Tabs helper active.

**Categories and prices**
Both versions: Abyss 94, Abyss Plans 93, Books 71, Recipes 95, Blueprints 122, Materials 41, Contracts 50 and Original 40, plus All. DLC adds its own tab with 234 expansion offers: 18 Abyss gears, 18 plans, 40 books, 19 recipes, 104 blueprints and 35 materials.
Long Horn and Water remain under Materials, with 1 Copper prices and 999 merchant stock each; normal inventory stack limits are unchanged. All 40 DLC crafting/knowledge books remain at Alden. Tiered Abyss Gear stays at Tier I; higher tiers remain for crafting/synthesis.
Counts describe the catalog, not guaranteed visible availability. Unlocks and sold-out conditions can reduce it; the current live DLC test showed 839 available offers. Three obsolete blueprint definitions were removed without renumbering persistent stock identities.
Original preserves Alden's 40 native offers, stocks and unlock/restock conditions. 31 keep normal prices; nine items shared with the mod catalog are 1 Copper in both places, with separate stock records. Reopening resets All. Labels support all 15 game text languages. Use mouse tabs or the displayed keyboard/controller controls.

**Requirements and installation**
Windows PC, Steam build 26636972, DMM 5.0.0 and its ASI loader, plus [Microsoft Visual C++ v14 Redistributable (x64)](https://aka.ms/vc14/vc_redist.x64.exe). DLC requires the installed expansion.

- Close the game and unmount in DMM.
- Completely remove/replace the previous Alden package and helper. Do not merge into an old mod folder.
- Import the complete ZIP for your chosen variant; enable the shop and its matching 1 Copper component.
- Enable the included Alden Category Tabs under ASI Mods. Update data and helper together. If DMM leaves the helper disabled, tick it; if it was not imported, extract only the included ASI and import it through ASI Mods.
- Mount again, restart and visit Alden.

Do not manually copy the internal files/ folder into the game. Mount again after installing the expansion or updating DMM.

**Compatibility and future updates**
The helper uses exact executable/UI guards. This release is for build 26636972 and is not update-independent. Old Better Inventory and Trade UI 3.3 compiled UI is not supported on this build; the previous blanket compatibility statement no longer applies. Other shop UI replacements require their own current-build validation.
The price component uses field edits to preserve unrelated item properties. Other mods changing Alden's store, the same prices or LocalStringInfo can still conflict. Other 439 merchant records are preserved by this release.

**Validation**
DLC shop and category switching were confirmed by the author. No DLC passed current catalog, UI-schema and native tests but has no separate current-build gameplay confirmation. Current UI conditions are serialized correctly and categories use persistent stock identities while leaving the game's purchase index intact. The release preserves the exact tested ASIs and game files. No DLC item definitions, models or textures are bundled.
For issues, include the game build, DMM version, active UI mods, Alden Category Tabs.asi.log and the mount log.

**Special thanks**
Thanks to the two community members who made the DLC available to me for building and testing this update.

**Uninstall**
Close the game, unmount, disable/remove the shop, price component and category helper, then remount. Purchased items and spent money are not rolled back.

Blablup / gutzufuss1477. Developed with AI assistance. MIT; MinHook license included.
[Source, release notes and checksums on GitHub](https://github.com/gutzufuss1477/Crimson-Desert-Mods).


[Current build and validation notes](../../releases/mods-b26636972.md).
