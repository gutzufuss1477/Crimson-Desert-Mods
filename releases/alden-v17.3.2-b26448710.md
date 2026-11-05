# Alden Shop 17.3.2 — Steam build 26448710

Released 5 November 2026. Regular shop update, confirmed in game by the author. [Nexus download](https://www.nexusmods.com/crimsondesert/mods/2066?tab=files&file_id=19702). Previous 17.3 is archived.

The previous category helper rejects the updated executable. Version 17.3.2 targets the installed Steam build 26448710 and rebases the shop table on its current data, preserving the game's change to another merchant. It retains the normal 567 mod offers at 1 Copper plus Alden's 40 original offers, all nine categories and 15 languages. DLC preview changes are excluded.

## Install or update

Close the game and unmount in DMM. Replace the previous Alden package completely, then import `Alden_Shop.zip`. Enable **Alden Shop**, **Alden Shop - 1 Copper**, and the included **Alden Category Tabs** ASI with DMM's ASI loader. Both the package and helper must be **17.3.2**. Keep only one Alden variant/helper active, mount and restart the game. The official Microsoft Visual C++ v14 x64 runtime remains required.

The ZIP includes the ASI helper. Original Better Inventory 3.3 remains separate and optional. No shared shop UI replacement or combined third-party package is included.

## Validation

- All 607 offer IDs exist in the current base-game data. Alden's complete offer record remains byte-identical to the confirmed 17.3 shop, including stock/save identities and original conditions.
- All 439 other merchants exactly match the current originals, including the changed `Store_Her_Costume` offer.
- All 706 field-level price edits have their exact intended effects on the current 7,069-item table. The 5,158 Equip All Armor field edits and Steelheart stamina change survive both application orders and repeat application.
- All 101,130 original translation records across 15 languages are retained. Existing category assets are unchanged.
- Native category and runtime UI regressions pass, covering nine categories, purchase/stock identity, original overlap items, shop reopening, vanilla/Better Inventory layouts, CSS loading, error paths and hook cleanup. Wrong-process refusal passes.
- 37 of 38 compared native function/data/vtable ranges are byte-identical at the same addresses as build 26423880. All 12 native guards match. The changed protected SelectTab jump retains the same unchanged caller and points into executable memory; its protected implementation has not been analyzed.
- ASI version metadata is 17.3.2.0. ASLR, DEP, Control Flow Guard and the shared Microsoft runtime are retained. Direct local Defender scans of both the ASI and ZIP completed without detections.

The author confirmed the supplied candidate works in game on build 26448710. The final ASI and all 20 game payloads are byte-identical to that candidate; only README.txt changed after confirmation. Separate reversed-load-order gameplay is not claimed. DLC runtime compatibility remains unverified. Executable checks remain active; future updates may require a new helper or refreshed shop table.

## Artifacts

| File | SHA-256 |
| --- | --- |
| `Alden_Shop.zip` | `8b90164dc3beefacc10489686d8145cdf35e07d29e2479932b9c60ecd799f1ba` |
| `Alden Category Tabs.asi` | `9983d998d98e8eb79b59ff344b7f76fbfb8b54d8d8fc7ac158914d613d2bac5d` |
| Supported `CrimsonDesert.exe` | `25f24e06a1f886e9310fd290f9462f66b480a5e7763d1fe156bfddaa8d1992b9` |

[Validation recipe](../recipes/alden-v17.3.2-b26448710/) · [Build source](../mods/alden-all-in-one-progression-shop/source/) · [Nexus mod page](https://www.nexusmods.com/crimsondesert/mods/2066)

Nexus file **19702** replaces archived file **19550**. Nexus virus scanning is in progress; download verification is pending. Full game-data packages remain Nexus-only; Git contains source, recipes, hashes and validation records.
