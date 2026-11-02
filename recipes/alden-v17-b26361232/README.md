# Alden v17 recipe — Steam build 26361232

This directory describes the published [Nexus v17 package](https://www.nexusmods.com/crimsondesert/mods/2066?tab=files&file_id=19088).

- `manifest.json`: exact DMM manifest from the release, including 706 price patches.
- `alden_catalog.tsv`: ordered catalog, with columns `order`, `item_id`, `stock` and no header. Contains 567 rows.
- `categories.json`: the same item IDs and order with each item's single category assignment, plus counts and localization keys for all eight tabs.
- `payload-hashes.json`: sizes and SHA-256 hashes of the released ASI, UI, localization and shop payloads.
- `validation.json`: publication and test evidence without local game data or private diagnostics.

The mod source contains the category helper, its native callback tests, the tab stylesheet and the 15 language-label sets. `catalog_generated.hpp` is a sorted ID-to-tab projection of `categories.json`.

The recipe preserves the confirmed stock quantities and purchase prices. Only Alden's shop is expanded. No DLC-exclusive items are included. Full modified game tables, language tables and compiled UI are distributed through Nexus, rather than this repository. The older `build_204` Alden workflow builds the historical 379-item version and does not reproduce this full v17 package.
