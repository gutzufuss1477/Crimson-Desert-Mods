# Alden v17.1 recipe — Steam build 26361232

Published [Nexus v17.1 package](https://www.nexusmods.com/crimsondesert/mods/2066?tab=files&file_id=19112), released 2 November 2026.

- `manifest.json`: exact release manifest; all 706 price patches are unchanged from v17.
- `payload-hashes.json`: released ASI, UI, localization and shop payload sizes and SHA-256 hashes.
- `validation.json`: test scope and package checksums.
- The [v17 catalog and category assignments](../alden-v17-b26361232/) remain unchanged: 567 items, eight tabs, the same order and stock.

The input fix changes only the two category keyguides' native press/release setting in HTML and compiled UI, plus the helper's corresponding UI hashes and diagnostic version text. See [source and regression check](../../mods/alden-all-in-one-progression-shop/source/). Both the data package and ASI must be updated.

The author confirmed the input-fix candidate works in game. Final game payloads are byte-identical to that candidate; the final ASI changes only its diagnostic version text. Separate physical keyboard/controller runs and the exact final ZIP were not independently confirmed. Native callback tests, isolated original-native input replay, artifact regression checks and wrong-process refusal passed.

Tier II/III Abyss Gear remains intentionally excluded from the shop. No DLC-exclusive items are added. Complete game data, localization and compiled UI remain Nexus-only under the repository's distribution policy.
