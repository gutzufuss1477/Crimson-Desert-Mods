# Nexus upload details

Mod name: **Bank Refresh - Configurable In-Game Timer**<br>
Version: **1.0.0**<br>
Author: **Blablup**<br>
Main file: **Bank_Refresh_2.03.02.zip**<br>
Supported game: **Crimson Desert 2.03.02 / Steam build 25474236**

## Short description

Customize the bank vault's refresh deadline in in-game minutes. Simple English INI, 15-minute default, no diagnostic logs. Requires an ASI loader and the supported game build.

## Description

Bank Refresh lets you shorten the recurring bank vault update timer without manually patching your save each time.

Choose your preferred interval in `Bank_Refresh.ini`. The default is **15 in-game minutes**, and values from 1 to 4320 are supported. The mod uses the game's calendar, including its different day/night progression, and leaves the actual bank update to the normal game code.

### Features

- Configurable interval in **in-game minutes**, not real-world minutes.
- Default: 15 in-game minutes.
- Simple English INI; restart the game after editing it.
- No diagnostic logs or background logging loop.
- No direct gold-quantity editing and no guaranteed profit.
- No EXE patch on disk; unsupported builds are rejected.

### Requirements and installation

Requires Crimson Desert **2.03.02 / Steam build 25474236** and an **x64 ASI loader** such as [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader). The loader is not included. Keep an existing working loader setup.

1. Close the game and back up your saves.
2. Remove older Bank Refresh ASIs. Do not run the old test builds alongside this release.
3. Copy `Bank_Refresh_2.03.02.asi` and `Bank_Refresh.ini` into the game's `bin64` directory next to `CrimsonDesert.exe`.
4. Start the game and load your save. Allow the next normal bank check to apply the interval.

This is a **manual ASI download**, not a JSON patch or a verified mod-manager installer.

### Settings

```ini
[BankRefresh]
Enabled=1
IntervalGameMinutes=15
```

Use whole numbers from **1 to 4320**. For example, 60 means one in-game hour and 1440 means one in-game day. Restart after changing the setting. Increasing it does not extend a shorter countdown already in progress; the longer setting applies after that short cycle ends.

### Why does the bank sometimes say "Updating"?

The game checks bank timers periodically rather than continuously. In testing, checks were about one real-world minute apart. An expired timer may wait until the next check, so short settings are not an exact payout frequency. This mod deliberately leaves that native check frequency unchanged.

### Compatibility and removal

Do not combine this mod with another bank timer mod or an older Bank Refresh test build. Only the recurring bank-vault path is modified; the separate timed investment-completion path is unchanged. Sleeping, fast travel and time mods can affect elapsed real time.

To uninstall, close the game and remove the ASI and INI. A shortened countdown can be stored by the game's normal save process; removing the mod does not restore that stored timer. Use a pre-mod save backup if needed.

### Tested behavior

Development builds were confirmed to refresh the bank and change gold bars. A custom 15-minute setting was also confirmed in-game. The public build removes logging and passes offline scheduling, configuration and load-guard tests; this exact log-free binary has not had a separate in-game run.

### Credits and source

Created by **Blablup**. Uses **MinHook** (license included).

[Source code and releases](https://github.com/gutzufuss1477/Crimson-Desert-Mods/tree/main/mods/bank-refresh)

## Upload checklist (not part of the description)

- Upload `release-assets/2.03.02/Bank_Refresh_2.03.02.zip` as the main file, version 1.0.0.
- State the x64 ASI loader requirement and the exact supported Steam build.
- Use the in-game screenshot only if you want to; screenshots are not bundled here.
- No files have been uploaded to Nexus by the assistant.
