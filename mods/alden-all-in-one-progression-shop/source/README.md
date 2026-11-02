# Alden localized category tabs

The source targets published **v17.1**, fixing skipped tabs with keyboard and Xbox LT/RT. The author confirmed the input fix in game. Use the release's matching data/UI package and ASI together.

`native/` contains the helper source, its generated game/UI guards and native callback tests for Steam build 26361232. Run `native/build.cmd` with Visual Studio C++ build tools installed. The script runs the callback tests and compiles the ASI.

`translations.json` contains the eight tab labels for each of the 15 language variants. `aldencategorytabs.css` contains the mod's tab layout rules.

The helper's filter and purchase behavior is unchanged from the user-confirmed 18c version. Localized labels and wrapping were subsequently confirmed in English in TEST 19. v17.1 updates its diagnostic text and UI hashes; filtering and purchasing code remain unchanged.

`fix_tab_input.py` applies the input correction to author-supplied v17 HTML and compiled UI, with exact input hashes and reversible, control-specific byte edits. Both category keyguides now explicitly use the native `need-trigger-downonce-release=True` setting. Their compiled v17 donor controls had retained the axis-control override `False`, causing DownOnce and Release to both activate the tab. No cooldown or custom input polling is added. Run `python test_tab_input.py path/to/v17.zip` for the artifact regression check. An isolated execution of the original game input routine reproduced two activations with the old attribute and one with the fixed attribute. The author subsequently confirmed the fix in game; physical input devices were not separately specified.

The [v17 recipe](../../../recipes/alden-v17-b26361232/) records the manifest, ordered stock list, category assignments and released payload hashes. It contains no complete game tables or compiled UI. The native helper can be built directly from this directory; building the complete data/localization/UI package additionally requires the author's local extraction and package-building workflow. The repository's historical data builder still targets the earlier 379-item Alden catalog.

The complete DMM package is published as v17.1 on [Nexus](https://www.nexusmods.com/crimsondesert/mods/2066?tab=files&file_id=19112). The [v17.1 recipe](../../../recipes/alden-v17.1-b26361232/) records its final manifest and payload hashes. All 22 game payloads are byte-identical to the confirmed input-fix candidate; the final helper changes only its diagnostic version text. All 15 language files were validated; English labels and wrapping were visually confirmed in game.
