# Mining Helmet Always On 1.1.0 validation

Date: 2026-09-28. Game: 2.03.02, installed Steam build 25474236.
Executable SHA-256: `57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`.

## In-game evidence

The user confirmed that the automatic Test34 build highlights ore and rocks,
and that gameplay actions are no longer blocked. The preceding controlled
material-only test specifically confirmed fast travel, character selection and
the quick wheel, with no green screen filter.

The tested ASI SHA-256 was
`3cde4699084583c590514593c0d955a96ef0f9c5e0e9ef14c697874c89458dab`.
Release 1.1.0 retains its runtime logic. Only the version string and diagnostic
directory/log prefix changed in `automatic_material.cpp`. The three implementation
headers, native tests and load-test host were copied unchanged. The build entry now selects
this material implementation and the game version in the public filename is
corrected to 2.03.02.

This is user-observed visual/gameplay validation, not a claim inferred solely
from event counts. Exhaustive coverage of every deposit, every special mode,
long sessions and combinations with other mods is not established. Increased
visibility distance is not part of this release.

## Technical checks

- Native C++ tests pass with MSVC x64, C++20, `/W4 /WX /O2 /MT`.
- Material write bounds, moving position/camera, restoration, foreign ownership,
  stale identities, native-mode yielding, fresh events, live-effect retention
  and original-source cleanup are covered by those tests.
- The release ASI loads in a separate x64 test host and records `wrong_process`,
  `Ready: no`, with zero game calls.
- All 14 guarded function entries and both hooked vtable slots match the installed
  executable. The ASI also checks its exact hash before installing hooks.
- No player SpecialMode writes or gameplay availability overrides are linked.
- Release-package tests verify ZIP members, checksums, the ASI copy and source
  hashes; historical packages and other mods are preserved.

The public ZIP contains only the ASI, README and short changelog. It contains no
game executable, decompiler dumps, test controllers, game logs or ASI loader.
