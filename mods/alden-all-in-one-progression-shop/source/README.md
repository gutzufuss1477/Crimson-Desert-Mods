# Alden localized category tabs

`native/` contains the helper source, its generated game/UI guards and native callback tests for Steam build 26361232. Run `native/build.cmd` with Visual Studio C++ build tools installed. The script runs the callback tests and compiles the ASI.

`translations.json` contains the eight tab labels for each of the 15 language variants. `aldencategorytabs.css` contains the mod's tab layout rules.

The helper's filter and purchase behavior is unchanged from the user-confirmed 18c version. Localized labels and wrapping were subsequently confirmed in English in TEST 19. Release v17 changes only the helper's diagnostic version string relative to that test.

The [v17 recipe](../../../recipes/alden-v17-b26361232/) records the manifest, ordered stock list, category assignments and released payload hashes. It contains no complete game tables or compiled UI. The native helper can be built directly from this directory; building the complete data/localization/UI package additionally requires the author's local extraction and package-building workflow. The repository's historical data builder still targets the earlier 379-item Alden catalog.

The complete DMM package is published as v17 on [Nexus](https://www.nexusmods.com/crimsondesert/mods/2066?tab=files&file_id=19088). All 15 language files were validated; English labels and wrapping were visually confirmed in game.
