# Alden All-In-One Progression Shop

Current release: **v17.2**, for **Crimson Desert 2.04.05 / Steam build 26361232**. Category tabs are added at runtime, so Alden works alongside the original **Better Inventory and Trade UI 3.3** without shared UI file replacements. Better Inventory is optional and installed separately; no combined package or required load order.

The author confirmed categories, purchasing, tab switching and no DMM file conflicts with original Better Inventory on a non-DLC installation on 3 November 2026. The final release preserves that candidate's runtime logic and all 20 game payloads; only diagnostic version text and packaging changed. Alden alone and reversed load order have technical validation, without separate gameplay confirmation. DLC installations and data tables remain unverified.

The catalog contains **567 items at 1 Copper**: All (567), Abyss (94), Abyss Plans (93), Books (71), Recipes (95), Blueprints (125), Materials (39) and Contracts (50). Labels follow the game text language, with all 15 variants included. Tab widths follow the label and can wrap onto a second row. The stock, 706 price patches and single-step keyboard/controller input from v17.1 are unchanged. Other merchant records are preserved.

Tiered Abyss Gear is deliberately offered at Tier I. Tier II/III, including Vitality II/III, are not sold, leaving higher-tier crafting to the player. No DLC-exclusive items are added.

[Download v17.2 on Nexus](https://www.nexusmods.com/crimsondesert/mods/2066?tab=files&file_id=19380)

[Changelog](CHANGELOG.md) · [Release notes and checksums](../../releases/alden-v17.2-b26361232.md) · [Manifest and validation recipe](../../recipes/alden-v17.2-b26361232/) · [Unchanged catalog](../../recipes/alden-v17-b26361232/)

## Install / update

Close the game and unmount in **DMM 4.6.4 or later**. Completely replace the previous Alden mod, including test builds. **Do not merge into an old Alden folder:** obsolete UI replacements must be removed. Import `Alden_Shop.zip`, enable Alden Shop and its 1 Copper price patch, and replace/enable the included **Alden Category Tabs.asi** in ASI Mods. Enable DMM's ASI loader, mount and restart. Only one Alden data mod and one category helper should be active. Do not manually copy the internal `files/` folder into the game.

If using [Better Inventory](https://www.nexusmods.com/crimsondesert/mods/452), follow its author's installation instructions, including the DLC-specific texture instructions. Alden supplies no Better Inventory files. Either order is valid for these two mods. Without it, the native 2×4 buy grid remains; with it, the original Better Inventory 3×5 grid remains.

To uninstall, close the game, unmount, disable Alden Shop, its price patch **and ASI**, then remount. Purchased items and spent money are not rolled back. Other shop/data, item-price or localization replacements can still conflict; unrelated shop UI replacements are not validated.

## Source and validation

[Source/build instructions](source/) describe the guarded runtime augmentation and native regression tests. The package contains 19 unchanged data/language files plus Alden's own stylesheet, with zero paths shared by the 36-file original Better Inventory package. Both overlay orders are byte-identical. All 15 language files passed data validation; English labels and wrapping were previously confirmed in game.

Complete game data/localization and generated game-derived fixtures stay outside Git. Download the complete package from Nexus; `nexus.json` records its version, file ID and checksum.
