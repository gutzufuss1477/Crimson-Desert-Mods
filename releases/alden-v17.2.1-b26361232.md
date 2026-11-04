# Alden All-In-One Progression Shop v17.2.1

Released **4 November 2026** for **Crimson Desert 2.04.05 / Steam build 26361232**. Requires **DMM 4.6.4+**.

[Nexus download](https://www.nexusmods.com/crimsondesert/mods/2066?tab=files&file_id=19467) · [Validation recipe](../recipes/alden-v17.2.1-b26361232/) · [Checksums](alden-v17.2.1-b26361232-SHA256SUMS.txt)

Fixes the outdated-patch warning when original Character Creator Enhanced — Equip All Armor changes item record sizes. Field-level edits replace fixed file positions. The original Character Creator mod stays separate and unchanged; no combined package is required.

All 567 items and 706 prices retain their previous effect. The catalog, stock, eight tabs, 15 languages and single-step input are unchanged. All 20 game payloads and the category ASI are byte-identical to v17.2; the helper intentionally still reports 17.2. Tier II/III Abyss Gear remains excluded. No DLC-exclusive items are added.

Completely replace old Alden packages and duplicate price patches. Import `Alden_Shop.zip`, enable Alden Shop, its price patch and the included category ASI, mount and restart. Original Better Inventory remains separate and optional, with no shared UI replacements or required load order.

The author confirmed Alden, Steelheart, original Character Creator Enhanced 9.1.32 / Equip All Armor and original Better Inventory 3.3 together in DMM and in game on 4 November 2026 without errors. The final packages preserve the confirmed field targets, effects and runtime payloads; only release metadata and readmes changed. The exact final ZIPs were not separately run in game. DLC installations/data remain unverified.

Data regressions verify identical original effects, both application orders, repeated application and preserved armor edits. The confirmed DMM mount applied all **5,855 item edits** from Equip All Armor, Alden and Steelheart. Independently parsed mounted data contain all **706 Alden prices at 1** and **Steelheart's buff level at 20**.

Published through Nexus API v3 as file **19467**; previous file **19380** archived. The downloaded archive matched the local release size and SHA-256 exactly. The horse page's other files and primary download remain unchanged.

Package: **4,526,806 bytes**; SHA-256 `037f2397b44a3654310a2d74e994dbc3adc35e39be9aec58ad14c9585fba39af`.

Full game tables, generated game-derived fixtures, parser binaries and third-party mod assets stay out of Git. The complete Alden game-data package is Nexus-only. Git contains source, narrow patch manifests, validation and release documentation.
