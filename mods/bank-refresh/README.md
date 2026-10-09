# Faster Bank Refresh - Gold and Bonds

Mod version: **1.1.0**<br>
Supported game: **Crimson Desert 2.03.02 / Steam build 25474236**<br>
Author: **Blablup / gutzufuss1477**

Shorten the regular gold-bank refresh timer and bond-investment duration independently, using **in-game minutes**. Both default to **15** in the supplied English INI.

**Bonds still require a manual start after each completed investment.** There is no automatic reinvestment. The game keeps control of gains, losses, investment strategies, completion and payouts. No balances are edited directly, and profits are not guaranteed.

## Requirements

- Windows x64 and the supported game executable.
- A working x64 ASI loader, such as [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader). Not included; keep your existing working loader setup.
- No other active Bank Refresh ASI or conflicting bank-timer modification.

Supported EXE SHA-256:
`57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`

Unknown executable builds or invalid enabled-section settings are rejected. A game update may require an updated mod.

## Installation and upgrading

1. Close the game and back up saves, normally under `%LOCALAPPDATA%\Pearl Abyss\CD\save`.
2. Disable/remove older Bank Refresh ASIs, including `Bank_Refresh_BR04.asi`, `Bank_Refresh_Bonds_Test1.asi` and older BR02/BR03 test versions. **Never load two Bank Refresh plugins together.** Leave your ASI loader and unrelated mods alone.
3. Extract `Bank_Refresh_2.03.02.asi` and `Bank_Refresh.ini` into the game's `bin64` folder next to `CrimsonDesert.exe`.
4. Use the supplied INI, or add the new `[Bonds]` section to your existing INI while preserving your preferred gold interval.
5. Start the game and load your save. Allow the next normal bank check to adjust an active timer.

This is a **manual ASI package**, not a JSON patch or a verified mod-manager installer. No loader, game files, saves, debug outputs or runtime logs are bundled. Existing logs from old test versions are not deleted.

## Configuration

```ini
[BankRefresh]
Enabled=1
IntervalGameMinutes=15

[Bonds]
Enabled=1
IntervalGameMinutes=15
```

- `[BankRefresh]` controls regular gold-bank refreshes: **1..4320** game minutes.
- `[Bonds]` controls active bond investments: **1..10080** game minutes.
- Each section has its own `Enabled`: `1` enables that timer adjustment; `0` leaves that bank unchanged. Disable both to disable all adjustments.
- Old INIs without `[Bonds]` keep the original **gold-only** behavior.
- Whole numbers only, without units or decimals. Examples: `60` = one game hour, `1440` = one game day.
- Restart after editing. Invalid enabled flags or intervals in an enabled section cause the mod to refuse activation.
- Only longer, active future deadlines are shortened. A shorter countdown is never extended; a longer setting takes effect on a later investment/cycle.

## Bond behavior

Unlock and use the bond bank normally. Deposit the required bonds and press **Start Investment** yourself. The mod shortens the active investment's deadline; it does not start an idle investment. After native completion, start the next investment manually, just as in the unmodded game.

No change is made to unlock requirements, minimum deposits, risk, profit/loss rules, caps, inventory or payout amounts.

## Timing and the "Updating" message

The game checks bank timers periodically, observed around once per real-world minute in earlier testing. A long countdown may remain briefly after loading or starting an investment. An expired countdown may show **Updating** until the next native check.

The INI controls a **deadline**, not an exact real-time payout frequency. Day/night and midnight transitions use the game's calendar. Sleeping, fast travel, pausing and time mods can affect elapsed real time.

## Uninstallation and compatibility

Close the game and remove the ASI and INI. A shortened deadline can be stored by the normal save system: uninstalling or disabling the mod does **not** restore an already shortened saved timer. Keep a pre-mod save backup and consider lost progress/Steam Cloud conflicts before restoring it.

Do not combine versions or other bank timer hooks. The game EXE is not patched on disk. Compatibility with other bank overhauls has not been established.

## Validation and source

Gold refreshes and a configurable 15-minute countdown were confirmed in-game. On 2026-10-09, the user confirmed the bond countdown dropped from several days to 15 game minutes, showed it counting down, and accepted the intended manual-start behavior.

The release preserves the accepted test's scheduling code and changes release metadata/filenames and documentation. The exact final binary has not had a separate playtest; long-term and save/load coverage remain limited.

Offline validation: **24,565** calendar/config checks, **284** host-side hook checks with engine doubles, **33** initialization cases and **2** production load checks without logs or test exports. See `reports/VALIDATION_BANK_REFRESH_1.1.0.md` in the repository.

Build on Windows with Visual Studio C++ Build Tools: `src\build.cmd`. Package from the repository with `scripts\Package-Bank-Refresh.ps1 -Force`; verify with `python -m pytest -q`. Build outputs remain under ignored `src/out`.

Original code: MIT, Copyright (c) 2026 Blablup. MinHook 1.3.4 retains its BSD-style license in `src/vendor/minhook/LICENSE.txt` and `MinHook-LICENSE.txt` in the ZIP. Thanks to the MinHook contributors and ThirteenAG for Ultimate ASI Loader.

[Nexus mod](https://www.nexusmods.com/crimsondesert/mods/3669) | [Source](https://github.com/gutzufuss1477/Crimson-Desert-Mods/tree/main/mods/bank-refresh)
