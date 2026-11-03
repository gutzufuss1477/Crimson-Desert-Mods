# Alden v17.2 recipe — Steam build 26361232

[Complete package on Nexus](https://www.nexusmods.com/crimsondesert/mods/2066?tab=files&file_id=19380). Released 3 November 2026.

- `manifest.json`: exact DMM browser-mod manifest.
- `price-patches.json`: exact 706 price patches, unchanged from v17.1.
- `payload-hashes.json`: all released file sizes and SHA-256 hashes.
- `validation.json`: author-confirmed gameplay scope and technical checks.
- The [v17 catalog](../alden-v17-b26361232/) retains all 567 items, ordering, stock and category assignments.

v17.2 adds category tabs at runtime without replacing shared shop HTML, compiled UI or shop CSS. The original Better Inventory 3.3 is optional and remains separate. Both file-overlay orders are identical and all 20 game payloads match the confirmed candidate; the final native change is diagnostic version text only.

The author confirmed categories, purchasing, tab switching and no DMM conflicts with Better Inventory without DLC. Standalone/reversed order have technical checks only. DLC installations/data tables and the exact final ZIP have not been separately gameplay-tested.

Complete game assets and generated game-derived fixtures remain outside Git. See the [runtime builder](../../mods/alden-all-in-one-progression-shop/source/).
