# Alden All-In-One Progression Shop

Current release: **v17.2.1**, for **Crimson Desert 2.04.05 / Steam build 26361232**. Fixes the outdated/partially skipped 1 Copper patch when original **Character Creator Enhanced — Equip All Armor** is enabled. All 706 price edits now target item IDs and fields, preserving armor changes when record sizes move. Both application orders passed data checks. Character Creator stays optional, separate and unchanged.

The v17.2 category system remains unchanged: original **Better Inventory and Trade UI 3.3** is optional and separate, with no shared UI replacement files or required load order. The included ASI is byte-identical to v17.2 and still reports that version.

The author confirmed Alden, Steelheart, original Character Creator Enhanced 9.1.32 / Equip All Armor and original Better Inventory 3.3 together in DMM and in game on 4 November 2026 without errors. The final packages preserve the confirmed field targets, effects and runtime payloads; only release metadata and readmes changed. The exact final ZIPs were not separately run in game. DLC installations/data remain unverified.

The catalog contains **567 items at 1 Copper**: All (567), Abyss (94), Abyss Plans (93), Books (71), Recipes (95), Blueprints (125), Materials (39) and Contracts (50). Labels follow the game text language, with all 15 variants included. Tab widths follow the label and can wrap onto a second row. Stock, the effect of all 706 prices and single-step keyboard/controller input are unchanged. Other merchant records are preserved.

Tiered Abyss Gear is deliberately offered at Tier I. Tier II/III, including Vitality II/III, are not sold, leaving higher-tier crafting to the player. No DLC-exclusive items are added.

[Download v17.2.1 on Nexus](https://www.nexusmods.com/crimsondesert/mods/2066?tab=files&file_id=19467)

[Changelog](CHANGELOG.md) · [Release notes and checksums](../../releases/alden-v17.2.1-b26361232.md) · [Manifest and validation recipe](../../recipes/alden-v17.2.1-b26361232/) · [Unchanged catalog](../../recipes/alden-v17-b26361232/)

## Install / update

Close the game and unmount in **DMM 4.6.4 or later**. Completely replace the previous Alden mod, including test builds. **Do not merge into an old Alden folder:** obsolete UI replacements must be removed. Import `Alden_Shop.zip`, enable Alden Shop and its 1 Copper price patch, and replace/enable the included **Alden Category Tabs.asi** in ASI Mods. Enable DMM's ASI loader, mount and restart. Only one Alden data mod and one category helper should be active. Do not manually copy the internal `files/` folder into the game.

If using [Better Inventory](https://www.nexusmods.com/crimsondesert/mods/452), follow its author's installation instructions, including the DLC-specific texture instructions. Alden supplies no Better Inventory files. Either order is valid for these two mods. Without it, the native 2×4 buy grid remains; with it, the original Better Inventory 3×5 grid remains.

To uninstall, close the game, unmount, disable Alden Shop, its price patch **and ASI**, then remount. Purchased items and spent money are not rolled back. Other shop/data, item-price or localization replacements can still conflict; unrelated shop UI replacements are not validated.

## Source and validation

[Source/build instructions](source/) describe the guarded runtime augmentation and native regression tests. The package contains 19 unchanged data/language files plus Alden's own stylesheet, with zero paths shared by the 36-file original Better Inventory package. Both overlay orders are byte-identical. All 15 language files passed data validation; English labels and wrapping were previously confirmed in game.

The real DMM mount applied all 5,855 item edits from Equip All Armor, Alden and Steelheart. The mounted table was independently checked: all 706 Alden prices equal 1 and Steelheart's buff level equals 20. See the release recipe for data-level regression results.

Complete game data/localization and generated game-derived fixtures stay outside Git. Download the complete package from Nexus; `nexus.json` records its version, file ID and checksum.
