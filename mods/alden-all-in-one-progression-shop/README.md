# Alden All-In-One Progression Shop

Current release: **v17.3**, for **Crimson Desert 2.04.05 / Steam build 26361232**. Adds an **Original** tab containing Alden's 40 native offers, alongside the existing 567 mod offers at 1 Copper.

The Original tab keeps the original stock limits and unlock/restock conditions. **31 original items retain their normal prices**; **nine shared items stay at 1 Copper** in both places, with separate stock records. Existing mod offers and stock identities are unchanged. Restored original offers receive new stock/save identities; historical vanilla purchase counters are not migrated.

The author confirmed the Original tab, native stocks and expected prices in game on **4 November 2026**. The final release keeps the exact confirmed ASI and all 20 game payloads; only the readme changed. DLC installations/data remain unverified.

Nine tabs follow all 15 game text languages: **All (607 offers / 598 distinct items), Abyss (94), Abyss Plans (93), Books (71), Recipes (95), Blueprints (125), Materials (39), Contracts (50), Original (40)**. Counts indicate catalog sizes; availability depends on game conditions and stock. Labels size/wrap automatically, and keyboard/controller switching retains the single-step input fix. Tiered Abyss Gear stays at Tier I; craft higher tiers yourself. No DLC-exclusive items are added.

[Download v17.3 on Nexus](https://www.nexusmods.com/crimsondesert/mods/2066?tab=files&file_id=19550) · [Changelog](CHANGELOG.md) · [Release notes and checksums](../../releases/alden-v17.3-b26361232.md) · [Validation recipe](../../recipes/alden-v17.3-b26361232/)

## Requirements and installation

Use **DMM 4.6.4+** with its ASI loader and the **[Microsoft Visual C++ v14 Redistributable, x64](https://aka.ms/vc14/vc_redist.x64.exe)**. Runtime DLLs are not bundled.

Close the game and unmount in DMM. Completely replace the previous Alden package; do not merge into an old folder. Import `Alden_Shop.zip`, enable **Alden Shop** and **Alden Shop - 1 Copper**, and replace/enable the included **Alden Category Tabs.asi** under ASI Mods. Both components must be **17.3**. Mount and restart. Keep only one Alden data mod, price patch and helper active. Do not manually copy the internal `files/` folder into the game.

## Compatibility

Original **[Better Inventory and Trade UI 3.3](https://www.nexusmods.com/crimsondesert/mods/452)** remains optional and separate. There are no shared UI replacement paths or required load order. Follow its author's installation instructions, including the DLC-specific texture rules. The native 2×4 buy grid remains without it; its 3×5 grid remains when installed.

Original **[Character Creator Enhanced 9.1.32 / Equip All Armor](https://www.nexusmods.com/crimsondesert/mods/837)** remains optional and unmodified. All 706 field-level price edits are unchanged, preserving armor changes in either application order. No combined mod or extra compatibility patch is required. Other mods changing the same shop data, prices or localization can still conflict.

The combined setup with Steelheart v10 was previously confirmed in DMM and in game without DLC. The Original feature has its own gameplay confirmation. Standalone/reversed-order cases have technical checks but no separate gameplay confirmation; DLC data remain unvalidated.

## Validation

Data checks preserve all 567 previous stock rows and every other merchant. The 40 original rows retain their native contents except for new stock/save identities. All existing localization records are retained. Native regressions cover nine tabs, all nine overlapping item pairs, separate purchase records, conditional availability, reopening, thread ownership, runtime UI loading and vanilla/Better Inventory layouts.

The final ASI and ZIP passed direct local Microsoft Defender scans on 4 November 2026 with signature **1.463.117.0**. The shared Microsoft runtime and Control Flow Guard build settings from 17.2.2 are retained. Reports against the older 17.2/17.2.1 helper were reproducible. No protection settings were changed; other antivirus engines and future signatures remain unverified. The ASI is unsigned.

## Uninstall and source

Close the game, unmount, disable Alden Shop, its price patch **and category ASI**, then remount. Purchased items and money spent are not rolled back.

[Build instructions and runtime behavior](source/) cover the exact game/UI guards and native tests. Full game-data packages remain Nexus-only. Git contains source, patch manifests, catalog/localization recipes, hashes and validation records.
