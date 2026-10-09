# Nexus release details

Title: **Faster Bank Refresh - Gold and Bonds**<br>
Version: **1.1.0**<br>
Author: **gutzufuss1477**<br>
Main file: **Bank_Refresh_2.03.02.zip**

## Short description

Shorten gold-bank refresh and bond-investment timers independently in in-game minutes. English INI, 15-minute defaults, no logs. Bonds still require a manual start. For Crimson Desert 2.03.02; x64 ASI loader required.

## Full description (Nexus BBCode source)

```bbcode
[b]Description[/b]
Shorten the regular gold-bank refresh timer and bond-investment duration independently, in IN-GAME minutes.

Version 1.1.0 adds support for the Anleihebank / bond bank. Both timers default to 15 in-game minutes and can be changed separately in the English INI.

[b]Bonds still need to be started MANUALLY after each completed investment.[/b] This is intentional vanilla behavior. There is no automatic start or reinvestment. The game still handles gains, losses and payouts; this mod does not edit balances directly or guarantee profits.

[b]Installation instructions[/b]
[list]
[*]Close the game and back up your saves.
[*]Install a working x64 ASI loader if you do not already have one. Keep an existing working loader setup.
[*]Disable/remove older Bank Refresh ASIs, including Bank_Refresh_BR04.asi and Bank_Refresh_Bonds_Test1.asi. Never run multiple Bank Refresh versions together.
[*]Copy Bank_Refresh_2.03.02.asi and Bank_Refresh.ini into the game's bin64 folder, next to CrimsonDesert.exe.
[*]When upgrading, use the updated INI or add the [Bonds] section shown below to your existing INI.
[*]Start the game and load your save. The interval applies on the next normal bank check, not necessarily immediately.
[/list]

[b]Configuration[/b]
[code][BankRefresh]
Enabled=1
IntervalGameMinutes=15

[Bonds]
Enabled=1
IntervalGameMinutes=15[/code]
[list]
[*][BankRefresh] controls regular gold-bank refreshes. Range: 1 to 4320 game minutes.
[*][Bonds] controls active bond investments. Range: 1 to 10080 game minutes.
[*]Each section has its own Enabled setting: 1 = enabled, 0 = leave that bank unchanged.
[*]An old INI without [Bonds] keeps the previous gold-only behavior.
[*]Use whole numbers, without units or decimals. 60 = one game hour; 1440 = one game day.
[*]Restart the game after changes. Invalid settings in an enabled section prevent activation.
[*]An existing shorter countdown is never extended. Longer settings apply to a later cycle/investment.
[/list]

[b]Using bonds[/b]
Unlock the bond bank normally, deposit the required bonds and select Start Investment. The mod only shortens an already active investment. After it completes, start another investment manually. Unlock requirements, minimum deposits, risk, investment strategy, gains/losses and payouts remain unchanged.

[b]Main features[/b]
[list]
[*]Separate configurable timers for gold-bank refreshes and bond investments.
[*]15 in-game minutes by default for each; independently enabled.
[*]Calendar-aware day/night and midnight handling.
[*]English INI and no diagnostic logs.
[*]Native processing and manual bond starts preserved.
[*]No permanent game EXE patch; unsupported executable builds are rejected.
[/list]

[b]About the "Updating" message[/b]
The game processes bank timers on its normal checks, observed around once per real-world minute during testing. A longer countdown may remain briefly after loading, and "Updating" may remain visible after zero until native processing runs.

The setting changes the deadline, not the native check frequency or an exact real-world payout interval. Sleeping, fast travel and other time changes can affect elapsed real time.

[b]Requirements[/b]
[list]
[*]Crimson Desert 2.03.02 / Steam build 25474236, supported Windows x64 executable.
[*]An x64 ASI loader, such as [url=https://github.com/ThirteenAG/Ultimate-ASI-Loader]Ultimate ASI Loader[/url]. Not included.
[*]No other active Bank Refresh version or conflicting bank-timer mod.
[/list]
This is a manual ASI package, not a JSON patch or a verified mod-manager installer. Game updates may require a new mod release.

[b]Removal and save safety[/b]
Close the game and remove the ASI and INI. Shortened deadlines may be stored by the game's save system; removing or disabling the mod will not restore those saved timers. Keep a pre-mod backup. Other bank overhauls have not been compatibility-tested.

[b]Tested behavior[/b]
Gold-bank refreshes and a configurable 15-minute countdown were confirmed in-game. Bond timer shortening and the intended manual-start behavior were confirmed in the accepted test build. Version 1.1.0 preserves that scheduling code with release metadata and updated packaging. The final rebuilt binary passed offline tests; it has not received a separate playtest. Long-term coverage remains limited.

[b]Shout outs[/b]
Thanks to Jicku12x and c47n1p for requesting bond support, to the MinHook contributors, and to ThirteenAG for Ultimate ASI Loader.

[url=https://github.com/gutzufuss1477/Crimson-Desert-Mods/tree/main/mods/bank-refresh]Source code and documentation on GitHub[/url]
```
