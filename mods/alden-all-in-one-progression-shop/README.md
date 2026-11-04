# Alden All-In-One Progression Shop

Current release: **v17.2.2**, for **Crimson Desert 2.04.05 / Steam build 26361232**. Rebuilds the category helper with the official shared Microsoft C++ runtime and complete version information. The final helper and ZIP pass local Microsoft Defender scans; shop functionality and the existing compatibility fixes are preserved.

The author confirmed the new runtime in game on **4 November 2026**. The final ASI changes only diagnostic/version metadata from that confirmed build, with native regressions and direct Defender scans repeated afterward. Both the package and helper report **17.2.2**. The exact final archive was not separately run in game; DLC installations/data remain unverified.

The shop offers **567 items at 1 Copper**: All (567), Abyss (94), Abyss Plans (93), Books (71), Recipes (95), Blueprints (125), Materials (39) and Contracts (50). All 20 game payloads and all 706 field-level price targets are unchanged from 17.2.1. Eight tabs follow the game text language across all 15 variants, with automatic widths/wrapping and single-step keyboard/controller input. Tiered Abyss Gear stays at Tier I; craft higher tiers yourself. No DLC-exclusive items are added.

[Download v17.2.2 on Nexus](https://www.nexusmods.com/crimsondesert/mods/2066?tab=files&file_id=19508) · [Changelog](CHANGELOG.md) · [Release notes and checksums](../../releases/alden-v17.2.2-b26361232.md) · [Validation recipe](../../recipes/alden-v17.2.2-b26361232/)

## Requirements and installation

Use **DMM 4.6.4+** with its ASI loader and the current **[Microsoft Visual C++ v14 Redistributable, x64](https://aka.ms/vc14/vc_redist.x64.exe)**. Microsoft runtime DLLs are not bundled.

Close the game and unmount in DMM. Completely replace the previous Alden mod, including old test versions; do not merge into an old folder. Import `Alden_Shop.zip`, enable **Alden Shop** and **Alden Shop - 1 Copper**, and replace/enable the included **Alden Category Tabs.asi** under ASI Mods. Mount and restart. Keep only one Alden data mod, price patch and helper active. Do not manually copy the internal `files/` folder into the game.

## Compatibility

Original **[Better Inventory and Trade UI 3.3](https://www.nexusmods.com/crimsondesert/mods/452)** remains optional and separate, with no shared UI replacement paths or required load order. Follow its author's installation instructions, including the DLC-specific texture rules. The native 2×4 buy grid remains without it; its 3×5 grid remains when installed.

Original **[Character Creator Enhanced 9.1.32 / Equip All Armor](https://www.nexusmods.com/crimsondesert/mods/837)** remains optional and unmodified. Alden's field-level price targets preserve armor changes in either application order. No combined mod or extra compatibility patch is needed. Other mods changing the same shop data, prices or localization can still conflict.

The combined setup with Steelheart v10 was previously confirmed in DMM and in game without DLC. All 706 price targets and the 20 game payloads are preserved in this release. Standalone/reversed-order cases have technical checks but no separate gameplay confirmation; DLC data remain unvalidated.

## Antivirus validation

Reports against the old 17.2/17.2.1 helper were reproducible. The final 17.2.2 ASI and ZIP each passed direct local Microsoft Defender scans on 4 November 2026 using signature **1.463.117.0**, which detected the old ASI. No protection settings were changed. This does not establish clearance by other engines or future signatures, and no vendor false-positive determination has been received. The ASI is unsigned. Report any remaining warning with its file name, antivirus product and detection name; do not disable protection to install it.

## Uninstall and source

Close the game, unmount, disable Alden Shop, its price patch **and category ASI**, then remount. Purchased items and money spent are not rolled back.

[Build instructions and runtime behavior](source/) cover the source, exact game/UI guards and native tests. The complete game-data package remains Nexus-only. Git contains source, patch manifests, catalog/localization recipes, hashes and validation records.
