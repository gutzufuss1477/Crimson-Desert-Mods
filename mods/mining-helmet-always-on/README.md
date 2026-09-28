# Mining Helmet Always On

Mod version: **1.1.0**<br>
Tested game build: **Crimson Desert 2.03.02 / Steam build 25474236**

Automatically highlights nearby ore and supported mineable rocks with the native
blue glow, whatever headgear you wear. No Mining Helmet or key press is required.

## What's changed in 1.1.0

The highlight now uses a separate material-rendering path. Earlier releases
kept the game's complete helmet mode active, which could block fast travel,
character selection and quick-wheel actions. This version does not activate
that gameplay mode or override the game's availability checks.

- Automatic blue glow after loading a save, without the green screen filter.
- Fast travel, character selection and normal quick-wheel actions remain available.
- Nearby targets are discovered automatically; the effect follows the camera.
- No timed test window, helmet capture or F7/F8/F9 controls.
- The native visibility and fade distance are unchanged.

The material approach was confirmed in-game with ore/rock glow and unrestricted
gameplay. The release changes only version/log labels from the successful Test34
implementation. Other game executables are not claimed to be supported.

## Install or update

1. Close the game.
2. **Remove the previous Mining Helmet ASI first**, including files named
   `Mining_Helmet_Always_On_2.03.00.asi` and any `test`/`direct-test` builds.
   If a mod manager installed the old version, disable/remove it there too.
3. Extract `Mining_Helmet_Always_On_2.03.02.asi` into the game's `bin64` folder.
4. Keep the ASI loader you already use. DMM is not required.
5. Start the game and load your save. Allow a few seconds for nearby resources
   to be detected. Do not press B or F8 to activate the mod.

Typical Steam folder:
`Steam\steamapps\common\Crimson Desert\bin64`

Only one version of this mod should be active. An ASI-capable mod manager may
manage the same file; manual installation requires an existing ASI loader.
The archive does not include a loader or any original game files.

## Native helmet and compatibility

The mod yields while a native special mode is active. If you turn on the actual
Mining Helmet with B, the game's original helmet effects and restrictions apply
until you switch that mode off again.

This release is guarded against the exact tested executable SHA-256:
`57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7`.
It refuses to activate on an unknown executable or conflicting hook slot.
Mods that control the same detection materials or effect handlers can conflict.

## Uninstall and diagnostics

Close the game and remove `Mining_Helmet_Always_On_2.03.02.asi` from `bin64`.
No save-file edits or original-file replacements are required.

Status: `%LOCALAPPDATA%\MiningHelmetAlwaysOn\MH110\latest-status.txt`.

## Source and build

The current implementation and tests are in `src/material/`.
Run `src/build.cmd` with Visual Studio 2022 C++ Build Tools installed. It builds
and runs the native tests, then loads the ASI in a separate test process to
verify its refusal to activate outside the game. Output: `src/material/out/`.

The historical mode-based sources remain in `src/` for reference and are not
linked by this build. Version 1.1.0 does not link MinHook.
Use `scripts/Package-Mining-Helmet.py` after the build to regenerate the ZIP,
binary copies and checksums. Release validation is recorded under `reports/`.
