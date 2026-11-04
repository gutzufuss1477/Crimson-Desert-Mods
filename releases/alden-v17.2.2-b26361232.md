# Alden All-In-One Progression Shop v17.2.2

Released **4 November 2026** for **Crimson Desert 2.04.05 / Steam build 26361232**. Requires **DMM 4.6.4+**, its ASI loader and the current **Microsoft Visual C++ v14 Redistributable (x64)**.

[Nexus download](https://www.nexusmods.com/crimsondesert/mods/2066?tab=files&file_id=19508) · [Validation recipe](../recipes/alden-v17.2.2-b26361232/) · [Checksums](alden-v17.2.2-b26361232-SHA256SUMS.txt)

The previous category helper caused reproducible Defender warnings. The replacement uses the official shared Microsoft runtime, complete version information and Control Flow Guard across all compiled objects, retaining ASLR/DEP. It has no functional C++ changes. The final ASI and package both report **17.2.2**, with no prerelease flag.

The author confirmed the rebuilt runtime in game. Final native differences from that build are diagnostic/version metadata only. Native category and runtime UI regressions, wrong-process refusal and direct local Defender scans passed after finalization. The exact final archive was not separately run in game.

The final ASI and ZIP returned **no threats found** with Defender engine **1.1.26100.4**, signature **1.463.117.0**, on 4 November 2026. That signature detected the old ASI. Antivirus settings were unchanged. Other vendors and future signatures remain unverified; there is no vendor false-positive verdict. The ASI is unsigned. This is a scoped scan result, not a general security certification.

All 20 game payloads, 567 items, 706 field-level price targets, stock, eight tabs and 15 languages are unchanged from 17.2.1. Original Better Inventory 3.3 and Character Creator Enhanced 9.1.32 / Equip All Armor remain separate, optional and compatible. No shared UI replacements or required Better Inventory load order are introduced. Tier I progression is retained. DLC installations/data remain unverified.

Replace the previous Alden package **and** category ASI, enable the data mod, 1 Copper patch and helper, then mount and restart. Install the [official Microsoft x64 runtime](https://aka.ms/vc14/vc_redist.x64.exe) if needed.

Package: **4,449,605 bytes**; SHA-256 `ab97e1f5e67348f4ab1763aaf3c03f0c1d945d48b5f59460a79a81ac8f57f3af`.

ASI: **83,456 bytes**; SHA-256 `3cc2566209ab1ea9e8692c04280cb61764a533356fc398867e151306b0400ec4`.

Nexus file **19508** replaces archived file **19467**. Publication and downloaded-file verification are recorded in the validation recipe. Full game assets, generated game-derived headers and third-party binaries stay outside Git.
