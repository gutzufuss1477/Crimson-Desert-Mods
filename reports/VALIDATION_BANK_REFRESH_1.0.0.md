# Bank Refresh 1.0.0 validation

Date: 2026-10-07. Target: Crimson Desert 2.03.02 / Steam build 25474236.

## In-game evidence and its boundary

- User confirmed repeated bank updates with changing gold bars in the active prototype.
- User then changed the game-time INI to 15 minutes and confirmed that the bank displayed that value after updating.
- The release preserves the calendar conversion, target callback, original-call order, actor-lock path, Bank_01 filter and deadline-only write from that tested implementation.
- Release changes: no event queue, runtime log files or heartbeat loop; initialization thread exits; English two-setting INI; default 15 game minutes; a production filename and version resource.
- This exact log-free release binary has **not** been independently run in-game. Prototype confirmation is not claimed as a separate release-binary playtest. Long-term and save/load coverage remain limited.

## Offline checks

- MSVC C++20 build with `/W4 /WX /O2 /MT /guard:cf`; PE uses ASLR/NX/CFG flags.
- 13,028 scheduling/configuration checks, including every starting minute of a day at eight intervals (including 15), day/night boundaries and midnight.
- 15 initialization guard cases in an instrumented test-only DLL: wrong process, missing/disabled config, bounds/format errors, old seconds/observe-only INIs, and unknown EXE.
- Two production ASI loads in isolated fake hosts. The production binary has no test API export and does not create or modify files in the test directory.
- Test host is a small purpose-built executable; the real game is not launched by tests.
- Release archive read back and verified against all six allowed members. No saves, game assets, diagnostic logs, debug build products or local analysis exports are packaged.
- Public repository tests verify archive hashes, English INI defaults, source provenance and absence of the test export/log filename in the release ASI.

## Timing caveat

The mod caps the deadline; it does not increase the frequency of the native bank callback. A periodic callback (observed around once per real minute) and a strict overdue comparison explain why "Updating" can remain visible after zero. The setting is not a promise of exactly one payout every 15 displayed game minutes.

## Build and release reproduction

1. Run `mods\bank-refresh\src\build.cmd` with Visual Studio C++ Build Tools available.
2. Run `scripts/Package-Bank-Refresh.ps1` (use `-Force` only to intentionally replace the existing generated package).
3. Run `python -m pytest -q`.

`BUILD_REPORT_BANK_REFRESH_1.0.0.json` records exact binary/archive hashes and source digests normalized to UTF-8 without BOM and LF line endings. Third-party MinHook source and license are retained in the source tree. No Nexus upload is performed by the build or package scripts.
