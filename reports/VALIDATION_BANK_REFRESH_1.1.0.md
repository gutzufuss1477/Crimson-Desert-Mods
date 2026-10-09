# Bank Refresh 1.1.0 validation

Date: 2026-10-09. Game: Crimson Desert 2.03.02 / Steam build 25474236.

## Runtime evidence and limits

- The original gold-bank implementation was confirmed to refresh gold bars and honor 15 game minutes.
- The user confirmed that a bond timer dropped from several days to 15 game minutes; their screenshot shows 9 minutes remaining and 10 bonds.
- The user identified manual investment starts, explicitly asked to preserve them, and accepted the mod as working.
- There is no separate instrumented capture of a bond payout, a second cycle, long sessions or save/load behavior. Do not represent the screenshot alone as proof of those cases.
- Release scheduling code is identical to the accepted bonds test. Only the source version comment, version resource, build/output names and package documentation change.
- The rebuilt final ASI is offline-tested; no separate final-binary playtest is claimed.

## Implementation boundary

Two independent INI sections and native callbacks. Original native handler called once before adjustment. Exact Bank_01/kind=0 or Bank_02/kind=1, scheduler return address, EXE SHA-256 and instruction guards. Only active future deadlines are shortened, under the native actor write guard. Inactive/due/completed/shorter timers are preserved. No automatic investment, balance edits, payout override or runtime log files.

Gold range remains 1..4320 game minutes; bonds accept 1..10080. Missing Bonds section defaults to disabled for backward compatibility. Supplied release enables both at 15.

## Offline checks

- MSVC C++20 /W4 /WX /O2 /MT /guard:cf build.
- 24,565 calendar/config/layout checks; full-day start-minute coverage, day/night boundaries, seven-day bond limit.
- 284 host-side hook checks using engine doubles, not real game execution.
- 33 startup guard cases and two production loads; no test export or file changes.
- Six-member release ZIP allowlist, readback, hashes and source provenance.
- Repository pytest validates current 1.1.0 assets and independent defaults.

Exact binary, ZIP and normalized source hashes are in BUILD_REPORT_BANK_REFRESH_1.1.0.json. Historical 1.0.0 assets/source are retained by Git history at commit ce94e4327bb9cc4e67245d8d106693f2ec28b612; the current game-version-named download now represents 1.1.0.

## Reproduction

1. Run mods/bank-refresh/src/build.cmd on Windows with C++ Build Tools.
2. Run scripts/Package-Bank-Refresh.ps1 -Force.
3. Run python -m pytest -q.

No proprietary game files, saved games, raw reverse-engineering exports, local deployment paths or secrets are included in the release.
