# Healthbar always on

**Release 3**, for **Crimson Desert 2.04.02 / Steam build 26329202**.

| Variant | Appearance | Targets | Included components |
|---|---|---|---|
| Vanilla Multitarget | Classic red bars with a dark frame | Multiple | DMM data + Healthbar Multitarget ASI |
| Classic Vanilla Single Target | Classic red bars with a dark frame | One | DMM data |
| Caites Multitarget | Compact thin red bars, no thick frame | Multiple | DMM data + Healthbar Multitarget ASI |

[Downloads on Nexus](https://www.nexusmods.com/crimsondesert/mods/2406?tab=files). Choose **one** variant. Each Nexus ZIP is a complete DMM import package; no separate helper download is needed.

## Installation

1. Close the game. Unmount and remove the previous Healthbar variant from DMM.
2. Import the complete ZIP with DMM 4.5.1 and enable the Healthbar data mod.
3. For Multitarget, also enable **Healthbar Multitarget** in **ASI Mods**. DMM imports it from the same ZIP, but may leave its checkbox off. Keep the existing loader, or install one through DMM if missing.
4. Mount and restart the game. No manual file copying or custom installer is required.

Close the game before switching variants; remove the old variant, import the new one and restart after mounting. The shared helper recognizes the selected style and does not patch Classic. Never enable duplicate helper copies. Remove older manually installed Healthbar test ASIs before migrating to DMM; retain unrelated mods, the ASI loader and DmmRuntime.asi.

To uninstall, close the game, disable/remove the Healthbar data and helper in DMM and mount the remaining mods.

## Validation and compatibility

Vanilla Multitarget's framed appearance, target switching and removal on death were confirmed with Kliff. Classic Single Target was separately confirmed working. Caites uses the exact previously confirmed compact UI payloads and the same native patch. Other characters and HP-value accuracy were not separately tested. The combined ZIP import was observed in DMM; DMM deployed the helper to bin64 and its startup log selected the expected UI mode.

The packages preserve the current 2,159-skill table except the intended ability, and apply three guarded character patches. Compatibility checks passed alongside Alden and Steelheart. The helper requires the exact supported executable and UI hashes; conflicting edits to subtitletagview.html/css or the same skill are unsupported.

## Source and build

The UI initializer requires ShowHPBuffTag; the Multitarget path also needed a guarded two-byte native branch patch. Vanilla and Classic share the framed geometry; only Caites uses compact geometry. Native eligibility, tracking and cleanup still run through the original UI update.

Build the helper with `source/native/build.cmd`; it runs the variant/atomic-patch/branch tests before compiling. The released `Healthbar Multitarget.asi` is the byte-identical NativeFix2 build, renamed for a readable DMM entry. Its SHA-256 is `6a8cd4012899d734016ece0f452349b21556e0bfea04c1302e41d36e380b5455`.

See [build instructions](../../tooling/README.md) for the local-input data builder and combined ZIP packager. Full game tables/UI assets, original inputs, executables and private diagnostics are excluded from GitHub. Ready-to-install complete packages are on Nexus; source, recipes and package hashes are recorded here.

Credits: Blablup / gutzufuss1477; [Caites - Persistent HP Bars](https://www.nexusmods.com/crimsondesert/mods/2409) for the original work and compact presentation. The native helper's MIT license applies only to original helper code.
