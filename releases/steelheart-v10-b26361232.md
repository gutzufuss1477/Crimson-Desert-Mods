# Steelheart Horseshoes v10

Released **4 November 2026** for **Crimson Desert 2.04.05 / Steam build 26361232**. Requires **DMM 4.6.4+**.

[Nexus download](https://www.nexusmods.com/crimsondesert/mods/2365?tab=files&file_id=19468) · [Validation recipe](../recipes/steelheart-v10-b26361232/) · [Checksums](steelheart-v10-b26361232-SHA256SUMS.txt)

Fixes the outdated-patch warning when original Character Creator Enhanced — Equip All Armor changes item record sizes. Field-level edits replace fixed file positions. The original Character Creator mod stays separate and unchanged; no combined package is required.

The equipped Steelheart Horseshoes stamina-regeneration buff still changes from level 4 to 20. Only item 1000594's intended buff field is edited; Equip All Armor changes are preserved. All Stats and Speed Only remain v13, with unchanged files and validation status.

Close the game and unmount. Replace/disable older Steelheart patches, import `Steelheart_Horseshoes.zip`, enable, mount and restart. Equip the horseshoes. No ASI is required; Alden and Character Creator are optional.

The author confirmed Alden, Steelheart, original Character Creator Enhanced 9.1.32 / Equip All Armor and original Better Inventory 3.3 together in DMM and in game on 4 November 2026 without errors. The final packages preserve the confirmed field targets, effects and runtime payloads; only release metadata and readmes changed. The exact final ZIPs were not separately run in game. DLC installations/data remain unverified.

Data regressions verify identical original effects, both application orders, repeated application and preserved armor edits. The confirmed DMM mount applied all **5,855 item edits** from Equip All Armor, Alden and Steelheart. Independently parsed mounted data contain all **706 Alden prices at 1** and **Steelheart's buff level at 20**.

Published through Nexus API v3 as file **19468**; previous file **18365** archived. The downloaded archive matched the local release size and SHA-256 exactly. The horse page's other files and primary download remain unchanged.

Package: **1,451 bytes**; SHA-256 `6e1d8586f317245598def8786a74f902f3cd0937ead032f796f8f635633bfbfe`.

Full game tables, generated game-derived fixtures, parser binaries and third-party mod assets stay out of Git. The complete Alden game-data package is Nexus-only. Git contains source, narrow patch manifests, validation and release documentation.
