# Alden v17.3 — Steam build 26361232

[Complete package on Nexus](https://www.nexusmods.com/crimsondesert/mods/2066?tab=files&file_id=19550). Released 4 November 2026.

- `manifest.json`: final DMM package metadata.
- `field-patches.json`: the unchanged 706 price effects with version 17.3.
- `categories.json`: 607 ordered offers / 598 distinct IDs, including 40 original offers and their price exceptions.
- `payload-hashes.json`: exact released entry sizes and SHA-256 hashes.
- `validation.json`: gameplay confirmation, readme-only finalization, tests and publication verification.
- `defender.json`: final ASI/ZIP scanner evidence.

All 20 game payloads and the ASI match the gameplay-confirmed package. The current source builder restores original stock from guarded local game inputs and checks it with the official DMM parser. Shared original/mod items keep independent stock identities; only their 1 Copper price is shared. Existing stock rows and localization records are preserved. Full original/modified tables and third-party binaries are not included here. DLC data remain unverified.
