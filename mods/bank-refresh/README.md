# Bank Refresh

Mod version: **1.0.0**<br>
Supported game: **Crimson Desert 2.03.02 / Steam build 25474236**<br>
Author: **Blablup**

Set the bank vault's recurring refresh deadline in **in-game minutes** using a simple INI file. The default is **15 in-game minutes**. The game still handles the actual bank update and its normal gold-bar changes; the mod does not force profits, edit gold quantities directly, or change investment strategy.

## Requirements

- The supported Windows x64 game executable (SHA-256 below).
- An x64 ASI loader, such as [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader). A working `winmm.dll` loader was used during testing. The loader is not included; do not overwrite an existing working loader setup.

Supported EXE SHA-256: `57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`.

The mod checks the complete executable hash and hook signatures. Unsupported builds and invalid settings are silently rejected. A game update may require a new mod release.

## Installation

1. Exit the game and back up your saves. The usual location is `%LOCALAPPDATA%\Pearl Abyss\CD\save`.
2. Remove or disable older Bank Refresh ASIs, including `Bank_Refresh_BR04.asi` and `Bank_Refresh_Test_BR02.asi` / `Bank_Refresh_Test_BR03.asi`. Never run two Bank Refresh plugins together. Do not remove your ASI loader or unrelated mods.
3. Extract `Bank_Refresh_2.03.02.asi` and `Bank_Refresh.ini` into the game's `bin64` folder, next to `CrimsonDesert.exe`. Keep the ASI and INI together.
4. Start the game and load your save. The first adjustment happens on the next normal bank update check, not necessarily immediately on loading.

Typical folder: `Steam\steamapps\common\Crimson Desert\bin64`.

This is a **manual ASI package**, not a JSON patch or a tested DMM/Vortex installer. No installer script, loader, original game files, saves, or diagnostic logs are included. The release plugin does not write logs and does not delete logs left by older test builds.

## Configuration

Open `Bank_Refresh.ini`:

```ini
[BankRefresh]
Enabled=1
IntervalGameMinutes=15
```

- `Enabled=1` enables the mod; `0` disables it after a restart.
- `IntervalGameMinutes` accepts whole numbers from **1 to 4320** (three in-game days).
- Examples: `15` = 15 game minutes; `60` = one game hour; `360` = six game hours; `1440` = one game day.
- Restart the game after changing the INI. Do not append units or use decimals.
- A shorter existing countdown is never extended. After increasing the setting, let that shorter cycle finish first.
- Old `IntervalSeconds` and diagnostic-mode settings are not part of this release configuration. Use the included INI when upgrading from a test build.

## Timing and the "Updating" message

The setting changes the refresh **deadline**, not the frequency of the game's bank checks. During testing those checks occurred about once per real-world minute, and a deadline equal to the current time was processed only on a later check. The bank may therefore display **"Updating"** after the countdown reaches zero. Very short settings do not guarantee an exact payout interval.

The calendar conversion handles day/night transitions and midnight. In-game minutes are not real-world minutes. Sleeping, fast travel, pausing, or other time mods can change elapsed real time.

Only the recurring `Bank_01` path is affected. The separate timed investment-completion path is left unchanged. Native gains and losses remain possible.

## Uninstallation and compatibility

Exit the game and remove `Bank_Refresh_2.03.02.asi` and its INI. Removing the mod does **not** undo a shortened timer already written by the game's normal save process; restore a pre-mod save backup if necessary. Be mindful of Steam Cloud conflicts when restoring saves.

Other bank timer mods or mods hooking the same functions can conflict. Never combine this release with the older Bank Refresh test plugins. The game's EXE is not patched on disk.

## Validation and source

The development builds were tested in-game: recurring updates changed the gold bars, and a user-selected **15-minute** setting appeared after a bank update. This release retains that scheduling/locking path, removes all runtime logging, and includes additional offline configuration and load-guard tests. The final log-free binary has not received a separate in-game run; extended stability testing remains limited.

Source and build instructions are in `src/`. On Windows with Visual Studio C++ Build Tools installed, run `src\build.cmd`. Build output and the instrumented test DLL stay under ignored `src/out`; the test export is absent from the release ASI.

Original mod code is MIT-licensed. MinHook is included under its own BSD-style license; see `MinHook-LICENSE.txt` in the download and `src/vendor/minhook/LICENSE.txt` in the source tree. No rights to game assets are granted.
