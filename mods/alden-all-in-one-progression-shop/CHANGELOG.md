# Alden AIO Shop + All Items 1 Copper

## v17.2.1 — 4 November 2026 — game 2.04.05

- Fix the outdated/partially skipped 1 Copper patch with original Character Creator Enhanced 9.1.32 / Equip All Armor.
- Replace fixed byte positions with 706 DMM field edits for the same 567 items and prices. Preserve the original armor changes in either application order.
- Keep the v17.2 category ASI, all game/localization payloads, stock, Tier I progression and Better Inventory compatibility unchanged.
- Combined DMM mount and gameplay confirmed with Steelheart v10, original Character Creator / Equip All Armor and Better Inventory 3.3 without DLC.
- Replace the previous Alden package and disable duplicate price patches. Requires DMM 4.6.4+. DLC data remain unverified.

[Download v17.2.1](https://www.nexusmods.com/crimsondesert/mods/2066?tab=files&file_id=19467). Previous v17.2 is archived. [Validation and checksums](../../releases/alden-v17.2.1-b26361232.md).

## v17.2 — 3 November 2026 — game 2.04.05

- Add category tabs at runtime instead of replacing shared shop HTML, compiled UI or shop CSS.
- Support the original, separately installed Better Inventory and Trade UI 3.3 without file conflicts or a required load order.
- Preserve the native buy grid without Better Inventory and Better Inventory's grid when installed.
- Preserve all 567 items, 706 price patches, stock, 15 languages and single-step tab input.
- Categories, purchasing and tab switching confirmed in game with original Better Inventory on a non-DLC installation.
- Update installation instructions: completely replace old Alden files and the matching ASI. DLC installations remain unverified.

[Download v17.2 on Nexus](https://www.nexusmods.com/crimsondesert/mods/2066?tab=files&file_id=19380). See the [release validation](../../recipes/alden-v17.2-b26361232/validation.json) for confirmation scope.

## v17.1 — 2 November 2026 — game 2.04.05

- Fixes the two compiled category keyguides retaining an axis-control press/release override, which made keyboard and Xbox LT/RT input skip a category.
- Uses the game's normal tab-key input setting; no artificial cooldown. Updates matching HTML and helper UI guards.
- Keeps all 567 items, prices, stock, layout and translations unchanged.
- Original native input replay, rapid repeated press/release tests, compiled UI regression checks and native callback tests pass. The author confirmed the fix in game.

Update both the data mod and included **Alden Category Tabs.asi**. Tier II/III Abyss Gear remains intentionally excluded from the shop; higher-tier crafting is left to the player.

[Download v17.1 on Nexus](https://www.nexusmods.com/crimsondesert/mods/2066?tab=files&file_id=19112). Published through Nexus API v3; v17 is archived. The downloadable archive was verified against the local SHA-256 checksum.

## v17 — 2 November 2026 — game 2.04.05

- Expands the previous 379-item catalog to 567 items from the installed base-game/update data, including rubber and additional crafting/progression items.
- Groups the inventory into eight tabs: All, Abyss, Abyss Plans, Books, Recipes, Blueprints, Materials and Contracts.
- Keeps item selection, stock handling and purchases connected to the original merchant records.
- Adds tab labels for all 15 game text-language variants, selected through the game's localization system.
- Fits tab widths to their labels and allows wrapping when a translation needs more space.
- Retains the 1 Copper pricing and the stock quantities of the tested expanded catalog.

Requires DMM and the included **Alden Category Tabs** ASI helper. Built specifically for Crimson Desert 2.04.05, Steam build 26361232. No DLC-exclusive items are included.

Category filtering and purchases were confirmed in game, followed by English localization and wrapping. All 15 language files passed data validation. The final package preserves the confirmed UI, catalog and language payloads; the helper changes only its diagnostic version string.

[Download v17 on Nexus](https://www.nexusmods.com/crimsondesert/mods/2066?tab=files&file_id=19088). The previous v16 file is archived for game 2.04.02.
