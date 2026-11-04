ALDEN SHOP 17.2.2
Crimson Desert 2.04.05 / Steam build 26361232; DMM 4.6.4 or later.
https://www.nexusmods.com/crimsondesert/mods/2066

567 items at 1 Copper, eight categories and 15 game text languages.
The category helper now uses the official shared Microsoft C++ runtime,
with version information and Control Flow Guard on all compiled objects.
Shop behavior, items, stock, language files and price targets are unchanged.
Original Better Inventory and Equip All Armor compatibility is retained.

REQUIREMENTS
Microsoft Visual C++ v14 Redistributable (x64), current supported release:
https://aka.ms/vc14/vc_redist.x64.exe
No Microsoft runtime DLLs are bundled. Enable DMM's ASI loader.

INSTALL / UPDATE
1. Close the game and unmount in DMM.
2. Completely replace the previous Alden package, including test versions.
   Do not merge into an old folder or keep duplicate price patches.
3. Import Alden_Shop.zip. Enable Alden Shop and Alden Shop - 1 Copper.
4. Replace/enable the included Alden Category Tabs.asi in ASI Mods.
   Keep only one Alden category helper active.
5. Mount, restart the game and visit Alden.

COMPATIBILITY
Same 567 items, 706 field-level price changes and all 20 game files as 17.2.1.
Original Better Inventory and Trade UI 3.3 and Character Creator Enhanced
9.1.32 / Equip All Armor remain separate and optional. No modified third-party
mod, combined package or compatibility patch is required. Follow their authors'
installation instructions, including Better Inventory's DLC texture rules.
Alden replaces no shared shop UI files; either Better Inventory order works.
Tiered Abyss Gear remains Tier I. No DLC-exclusive items are added.
Other mods changing the same shop data, prices or localization may conflict.

VALIDATION
The author confirmed the new runtime in game on 4 November 2026. The final
helper keeps the confirmed functional code; only its diagnostic/version
metadata was finalized to 17.2.2. All game files and price targets are unchanged.
Native category and runtime UI tests passed, including vanilla/Better Inventory
fixtures, purchase identity, single-step input and wrong-process refusal.
DLC installations/data remain unverified.

The previous ASI's Defender warning was reproducible. The replacement ASI
passed a direct local Microsoft Defender scan on 4 November 2026 with
signature 1.463.117.0, which detected the old ASI. This is not a guarantee
for other antivirus engines or future signatures; no vendor false-positive
determination has been received. Antivirus settings were not changed.
The ASI is unsigned. Report any remaining warning with the file name,
antivirus product and detection name; do not disable protection to install it.

UNINSTALL
Close the game, unmount, disable Alden Shop, its price patch and category ASI,
then remount. Purchased items and money spent are not rolled back.

Blablup / gutzufuss1477. Developed with AI assistance. MIT; MinHook license
included. Source: https://github.com/gutzufuss1477/Crimson-Desert-Mods
