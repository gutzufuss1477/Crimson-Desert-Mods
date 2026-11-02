# Alden All-In-One Progression Shop

Current release: **v17**, for **Crimson Desert 2.04.05 / Steam build 26361232**. Tested with DMM 4.6.4. Category filtering and purchases were confirmed in game, followed by English labels and wrapping on 2 November 2026.

The catalog contains **567 items at 1 Copper**, grouped into eight tabs: All (567), Abyss (94), Abyss Plans (93), Books (71), Recipes (95), Blueprints (125), Materials (39) and Contracts (50). Labels follow the game text language, with all 15 variants included. Tab widths follow the label and can wrap onto a second row. Other merchant records are preserved. No DLC-exclusive items are added.

[Download v17 on Nexus](https://www.nexusmods.com/crimsondesert/mods/2066?tab=files&file_id=19088)

[Changelog](CHANGELOG.md) · [Release notes and checksums](../../releases/alden-v17-b26361232.md) · [Catalog and manifest recipe](../../recipes/alden-v17-b26361232/)

## Install / update

Close the game and unmount in DMM. Disable previous Alden packages, including test versions. Import `Alden_AIO_Shop_All_Items_1_Copper_2.04.05.zip`, enable the data mod, and import/replace and enable the included **Alden Category Tabs.asi** in DMM's ASI Mods section. Both components are required. Enable DMM's ASI loader, mount, restart and visit Alden. Do not copy the ZIP's internal `files/` folder manually.

To uninstall, close the game, unmount, disable the Alden data mod **and ASI helper**, and remount the remaining mods. Purchased items and money spent are not rolled back. Other Alden/shop, shop-UI, item-price or LocalStringInfo/localization replacements may conflict.

## Source and validation

`source/native/` contains the helper and its native callback tests. All 15 language files passed data validation; English labels and wrapping were confirmed in game. Existing localization records, the original purchase widgets and non-target merchant records are preserved. Original and complete modified game tables stay outside Git; get the complete package from Nexus. `nexus.json` records its file ID, version and checksum.
