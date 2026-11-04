# Alden v17.3 source

v17.3 adds an Original tab containing Alden's 40 native offers. The existing 567 mod offers retain their stock identities and 1 Copper prices. Original limits and unlock/restock conditions are retained; 31 original items keep their normal prices and nine shared items retain the existing 1 Copper price. The new offers have separate stock/save indices 567–606. Historical vanilla purchase counters are not migrated. There are 607 offers, 598 distinct items and nine tabs; conditional or sold-out offers may be hidden. Tab counters show catalog sizes.

The [field converter](../../../tooling/crimson_mod_tools/field_patches_204.py) exposes `alden_prices` and `steelheart_stamina`. Supply the official DMM parser, the vanilla ItemInfo body, legacy manifests and the catalog item IDs. Conversion requires an exact parser round-trip and byte-identical effects to the original patches. No parser binary or third-party assets are redistributed. The parser binding used for game 2.04.05 comes from [dmm-parser commit cb1296e](https://github.com/exodiaprivate-eng/dmm-parser/commit/cb1296e8261818e0f6661a21de258ae2cdc8a161); later schemas may target a different game build.

Run the composition regression from the repository root with local, legally obtained inputs:

```powershell
python tooling/test_field_patches_204.py --parser-python C:\Inputs\parser --body C:\Inputs\iteminfo.staticinfobody --alden-prices C:\Inputs\Alden17.2-Preise.json --catalog recipes/alden-v17-b26361232/categories.json --steelheart C:\Inputs\Steelheart9.json --equip-all C:\Inputs\EquipAllArmor.json
```

The regression checks exact original effects, both application orders, repeat application, preserved armor edits and rejection of changed original data. The [v17.2.1 recipe](../../../recipes/alden-v17.2.1-b26361232/) contains the exact release manifest, hashes and current confirmation scope. Combined gameplay was confirmed on 4 November 2026; DLC data remain unverified.

## v17.2 category runtime

The ASI inserts nine category controls into the supported buy UI in memory. The package contains 19 Alden data/language files and its own `aldencategorytabs.css`. None of its 20 game paths overlaps the 36 original Better Inventory 3.3 paths. Both DMM load orders therefore produce identical effective files. No Better Inventory assets are bundled.

The native 2×4 buy grid remains without Better Inventory; the original Better Inventory 3×5 grid remains when installed separately. Replace old Alden files and its matching ASI completely when updating. See the [installation instructions](../).

## Build

Use Python with Capstone, lz4, the official DMM parser binding and Visual Studio x64 C++ build tools, including the Windows SDK resource compiler:

```powershell
python build_runtime_shop.py --alden C:\Inputs\Alden17.1.zip --better-inventory C:\Inputs\BetterInventory3.3.zip --vanilla-ui C:\Inputs\npcstorebuyview2_game.uibinary --vanilla-data C:\Inputs\b26361232 --parser-dir C:\Inputs\parser --exe C:\Inputs\CrimsonDesert.exe --output C:\Output\AldenRuntime
```

The game executable, UI/archive inputs and original StoreInfo body/header are checked against exact hashes for game 2.04.05 / Steam 26361232. Better Inventory is a build-time verification input, not a runtime dependency. The build first verifies the known eight-tab trees, adds only the ninth header and All counter, then checks reversibility and the unchanged buy-grid subtree. `original_shop.py` appends the original offers, independently parses both tables with DMM, preserves all other merchants, and verifies that only the nine shared original items occur in the price recipe. Existing localization records and offsets are retained while one new label is appended in all 15 languages. Generated game-derived headers, fixtures and complete game assets belong in the local output directory, outside Git.

`runtime_build_inputs.py` provides read-only archive and dependency-list helpers. `ui_binary.py` decodes the build-specific tree for validation. `native/build_runtime.cmd` runs in the generated output directory and compiles the helper and regression hosts. The build reuses licensed [MinHook 1.3.4 source](../../mining-helmet-always-on/source/src/vendor/minhook/) and includes its license. `native/build.cmd` remains the legacy v17.1 build entry.

The working-tree runtime build now uses `/MD` consistently for the helper, MinHook and regression hosts, with Control Flow Guard on every object and ASLR/DEP retained. It uses the official shared Microsoft runtime instead of embedding a separate static copy. Install the current [Microsoft Visual C++ v14 Redistributable, x64](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist) when needed. Runtime DLLs are not bundled. `version.rc` reads the generated `version_generated.h` for honest product/version metadata; this is not an Authenticode signature.

The current builder produces **17.3** for both package and helper, without a prerelease flag. It reuses the validated 17.2.1 field-level price targets. Stock and localization files change for the Original tab; no ItemInfo replacement or shared UI file is introduced. The author confirmed the Original tab, native stocks and price exceptions in game on 4 November 2026. The release retains that exact ASI and all game payloads; only the package readme changed.

## Runtime behavior

The UI decoder hook at RVA `0x41e6d90` recognizes exact vanilla or original Better Inventory 3.3 buy trees. Four reversible edits insert Alden's tab subtree and root selector/counts. The original grid, other UI bytes and all 46 stylesheet dependencies remain unchanged. A per-call borrowed memory reader supplies the augmented bytes to the original decoder once; the loader retains ownership of the source buffer. The original cursor advances only after successful complete decoding. Unsupported UI passes through unchanged.

When the native loader at `0x41bbf30` loads `NpcStore2.css`, the helper compiles its authored CSS through `0x41ac150` and registers/binds it through `0x41a7e60`. Cached CSS uses the native reuse path. The compiler receives a private mutable copy, and compiled output is released through the game's destructor. This follows the observed load order: UI preloading can precede stylesheet initialization.

Category callbacks activate only after successful style registration. Optional style failure preserves native view construction, hides the category bar and retains the complete unfiltered shop. Executable hash, function bytes, memory-reader vtable and mounted category CSS are guarded. Legacy Alden buy-UI replacements are refused, exposing an incomplete upgrade. Hook activation rolls back on failure. Existing category filtering, purchase identity, localized labels and single-step input remain in use.

## Validation

The previous runtime and shared-runtime rebuild were confirmed in game on a non-DLC installation on 3–4 November 2026. The author separately confirmed the Original tab, native stocks and expected prices on 4 November 2026. Save/reload stock persistence was not separately reported.

Native regressions cover category callbacks, purchase/stock identity, reopen, merchant isolation, thread ownership, reentrancy, unsupported/truncated input, EOF/decode failures and nested/concurrent loads. The isolated host executes the original 64-byte memory-read routine, actual dependency-consumer branch flow and real MinHook redirects/trampolines/cleanup. The dependency consumer reproduces the old missing-CSS abort and verifies normal UI binding with late CSS registration, including optional style failure. CSS compiler/decoder and renderer calls are fixtures, not a complete game execution.

Static checks preserve all 706 price effects and the existing 567 stock rows, restore exact input trees when UI edits are reversed and show no shared paths with original Better Inventory. New native regressions cover all nine overlapping items, separate original/mod purchase pointers, sparse conditional stock and rejection of unfamiliar stock indices. The native mounted-file guard accepts standalone/Better Inventory fixtures and rejects missing CSS or obsolete Alden UI. The helper refuses loading into an unrelated process.

Alden alone and reversed load order lack separate gameplay confirmation. No DLC installation/tester was available; DLC data tables are unvalidated. Optional-texture fixture checks do not prove DLC compatibility. No DLC-exclusive content is added. See the [release recipe](../../../recipes/alden-v17.2-b26361232/).

## Catalog, localization and input provenance

The [v17 catalog](../../../recipes/alden-v17-b26361232/) remains unchanged within its existing tabs: 567 items, eight original categories and deliberate Tier I limits for tiered Abyss Gear. The ninth Original tab is derived from the guarded vanilla StoreInfo table. `translations.json` holds all 15 language variants; `aldencategorytabs.css` defines label sizing/wrapping. The previous eight-tab English labels/wrapping were confirmed in game; the ninth tab was confirmed present and functional in game.

The v17.1 `fix_tab_input.py` / `test_tab_input.py` artifacts document the retained input fix. Both tab keyguides use the native `need-trigger-downonce-release=True` setting, preventing press and release from advancing twice. No artificial cooldown or input polling is added. Historical versions remain in Git history and their release recipes. Earlier local compatibility experiment builders are not release entry points or dependencies.
