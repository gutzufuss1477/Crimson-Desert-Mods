# Alden release hotfix recipe — Steam 26480954

This recipe records regular **17.3.3** and optional DLC **17.4.2**, published on 6 November 2026. It updates the existing catalog releases and contains no Alfred feature.

Use [build_release_hotfix.py](../../mods/alden-all-in-one-progression-shop/source/build_release_hotfix.py) with the inputs documented in the [source instructions](../../mods/alden-all-in-one-progression-shop/source/). The builder requires the exact previous published ZIP and its generated native build directory, current extracted base data, DMM parser, exact supported executable, original Better Inventory 3.3 ZIP and MSVC build environment. Generated native headers and game payloads are not distributed in Git. This repository alone is not a complete set of build inputs.

`native-profile.json` records the executable hash, verified addresses, evidence and limits. `input-hashes.json` records 29 current official source files. The previous packages are checked against fixed hashes before building; the catalogs, price targets and runtime logic are retained.

| Record | Meaning |
|---|---|
| `regular-validation.json`, `dlc-validation.json` | Native/UI regressions, catalog/data preservation, output hashes and runtime limits |
| `regular-composition.json`, `dlc-composition.json` | Price patches composed with original Equip All Armor and Steelheart in both orders, including repeat application |
| `regular-defender.json`, `dlc-defender.json` | Direct final ZIP/ASI scan results |
| `regular-published_nexus.json`, `dlc-published_nexus.json` | Published file IDs, archive state, output hashes and downloaded-file verification |
| `nexus-description.bbcode.txt` | Published page text, verified by API readback |

The build runs its native test hosts. The DLC catalog/composition checks use [test_dlc_shop.py](../../mods/alden-all-in-one-progression-shop/source/test_dlc_shop.py) with `--expected-version 17.4.2` and the corresponding package/reference inputs. Composition reports concern independently applied field patches, not a full DMM/game test.

**No current-build gameplay confirmation is claimed.** The author has no DLC. Current DLC tables and full DLC runtime behavior remain unverified; the separate community crash report is not reproduced. A clean local antivirus scan does not cover other engines or future signatures.

[Published downloads, release notes and checksums](../../releases/alden-hotfix-b26480954.md)
