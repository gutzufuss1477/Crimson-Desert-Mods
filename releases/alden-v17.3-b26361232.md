# Alden All-In-One Progression Shop v17.3

Released **4 November 2026** for **Crimson Desert 2.04.05 / Steam build 26361232**. Requires **DMM 4.6.4+**, its ASI loader and **Microsoft Visual C++ v14 Redistributable (x64)**.

[Nexus download](https://www.nexusmods.com/crimsondesert/mods/2066?tab=files&file_id=19550) · [Validation recipe](../recipes/alden-v17.3-b26361232/) · [Checksums](alden-v17.3-b26361232-SHA256SUMS.txt)

Adds the **Original** tab with Alden's 40 native offers, original stock limits and unlock/restock conditions. 31 original items retain normal prices; nine shared items stay at 1 Copper with separate stock records. The existing 567 mod offers, stock identities, 706 field-level prices and Tier I progression remain unchanged. There are nine localized tabs and up to 607 offers / 598 distinct items.

The author confirmed the Original tab, native stocks and expected prices in game. The final ASI and all 20 game payloads are byte-identical to the confirmed package; only `README.txt` changed. Historical vanilla purchase counters are not migrated to the new stock identities. Save/reload persistence was not separately reported.

Native callback/runtime regressions and independent DMM data parsing passed. No shared replacement paths with original Better Inventory 3.3 are introduced, and existing Equip All Armor price edits remain unchanged. Separate standalone/reversed-order gameplay and DLC installations/data remain unverified.

Final ASI and ZIP Defender scans returned **no threats found**, engine **1.1.26100.4**, signature **1.463.117.0**. The shared-runtime/Control Flow Guard build settings from 17.2.2 remain. Protection settings were unchanged; other antivirus engines are unverified. No new binary changes were made after gameplay confirmation.

Replace the old Alden package **and category ASI**, enable the data mod, 1 Copper patch and helper, then mount and restart. Both components report **17.3**. Original third-party mods stay separate.

Package: **4,450,500 bytes**, SHA-256 `9ef0f129764d47f398c04eba47e9a87c4a611395f7c8f4b9bf4a2c2c25ec0da8`.

ASI: **84,480 bytes**, SHA-256 `99d814cd7152052086e1cb841d9d1c77d6682377735947dd72c8aa1e3eee50ac`.

Nexus file **19550** replaces archived file **19508**. The published download was retrieved and its SHA-256 verified. Full game assets, generated game-derived headers and third-party binaries stay outside Git.
