# Alden All-In-One Progression Shop

Current release: **v16**, for **Crimson Desert 2.04.02 / Steam build 26329202**. DMM 4.5.1 mount and gameplay confirmed by the author on 31 October 2026.

The original 379-item catalog, stock and order are preserved: progression blueprints, books, contracts, Abyss gear, materials and recipes. Catalog purchase prices are 1 Copper. Only Alden changes among 440 store records; all other records remain byte-identical.

[Download v16 on Nexus](https://www.nexusmods.com/crimsondesert/mods/2066?tab=files&file_id=18366) · [Release notes and checksums](https://github.com/gutzufuss1477/Crimson-Desert-Mods/releases/tag/mods-2.04.02-b26329202)

## Install / update

Close the game and unmount in DMM. Import `Alden_AIO_Shop_All_Items_1_Copper_2.04.02.zip` with DMM 4.5.1 or a compatible newer version. Disable/remove the previous Alden package, enable the new one and mount. Keep DMM's installed runtime and ASI loader. Restart and visit Alden. Do not copy the ZIP's internal `files/` folder manually.

To uninstall, close the game, unmount, disable Alden and remount the remaining mods. Purchased items and money spent are not rolled back. Other Alden/shop overlays or item-price edits may conflict.

## Source and validation

Current recipes are in `recipes/2.04.02`. The builder preserves all non-target store records and checks all 502 purchase-price edits against exact vanilla input hashes. See [build instructions](../../tooling/README.md). Original and complete modified game tables stay outside Git; get the ready-to-install data package from Nexus. `nexus.json` records the exact file ID, version and checksum.
