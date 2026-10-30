# Build and validation

The native release targets Crimson Desert 2.04.00, Steam build 26317560. Build with Visual Studio C++ x64 tools and the Windows SDK:

```bat
mods\mining-helmet-always-on\source\src\build.cmd
mods\faster-bank-refresh-gold-and-bonds\source\src\build.cmd
tooling\build_allstats_204.cmd
tooling\build_speed_204.cmd
```

Each command runs its host tests. Mining covers configuration, lifecycle, camera/material handling and ore range; Bank covers scheduling, hooks and initialization; both mount variants cover eligible mount stats, rejected records and exception handling (1,135 checks). Loader tests reject unsupported hosts. Native byte/layout checks and hooks were also checked against the target executable and a running game. The author confirmed Mining Helmet, Bank Refresh and AllStats working in gameplay on 30 October 2026. Speed-only has the identical runtime hook with a narrower stat filter and passed 1,135 host checks plus loader checks; a separate gameplay test of Speed-only is pending.

The first three release payloads retain the exact gameplay-tested ASI bytes; Speed-only retains its separately checked build payload. To package those approved build outputs using Python's standard library:

```powershell
python tooling/package_native_release.py --output dist/published-2.04.00-b26317560-r2
```

The packager refuses different ASI hashes and writes four ZIPs plus SHA256SUMS.txt. A fresh compiler/toolchain may produce different bytes; such a build needs its own validation before changing the approved hashes. BUILD_VALIDATION.txt in each ZIP records the payload hashes and the tested executable identity. No original game inputs or third-party loader binaries are distributed.

## Historical data-overlay tooling

The original shared table/parser tooling retains its scoped license. Its historical default is 2.00.00; it is not a builder for this native release or a claim of 2.04.00 data-mod compatibility. Full game tables and other game-derived inputs must remain local.
