# Alden All-In-One Progression Shop v17.1

Released **2 November 2026**, for **Crimson Desert 2.04.05 / Steam build 26361232**.

[Complete package on Nexus](https://www.nexusmods.com/crimsondesert/mods/2066?tab=files&file_id=19112) · [Source and installation](../mods/alden-all-in-one-progression-shop/) · [Manifest and validation](../recipes/alden-v17.1-b26361232/) · [SHA-256 checksums](alden-v17.1-b26361232-SHA256SUMS.txt)

Fixes category tabs skipping one entry with Xbox LT/RT and keyboard tab controls. The two category keyguides now use the game's native press/release setting, so a short press advances one category without an artificial cooldown. **Update both the DMM data package and the included Alden Category Tabs.asi.** Requires DMM 4.6.4 or later with its ASI loader enabled; disable older Alden packages/helpers, mount and restart.

All **567 items**, 1 Copper prices, stock, eight categories, layout and 15 language variants are unchanged from v17. Tier II/III Abyss Gear is deliberately not sold, leaving higher-tier crafting to the player. No DLC-exclusive items are added.

The author confirmed the input-fix candidate in game. All 22 final game payloads match that candidate byte for byte; the final ASI changes only its diagnostic version text. Native callback tests, rapid native press/release replay, compiled-UI regression checks and wrong-process refusal passed. Input devices were not separately specified in the gameplay confirmation, and the exact final archive was not independently tested in game.

Published through **Nexus API v3** as file **19112**, Main/Primary. Previous v17 file 19088 is archived. The downloaded release matched the local archive: **4,450,759 bytes**, SHA-256 `f4b7b6205074e11f92b621e99f4da959f61ff2b2098328016d99009e6925bd3b`. Helper SHA-256: `93f5ee2a493575a800c92cd7d0b00f112e2d04f4a89138aba467e9addcfb9c02`.

Git contains authored source, the patch recipe, regression check and release documentation. Complete game-data/UI packages remain Nexus-only.
