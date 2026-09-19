# All Mounts LvL 5 Speed

Current version: **2.03.00**

Sets only the compatible mount **Movement Speed** rank to Level 5.

Acceleration, Turning / Handling and Jump remain unchanged.

## 2.03.00 implementation

Crimson Desert 2.03.00 changed how active mount stats are resolved for existing mounts.
The previous static CharacterInfo data patch is therefore replaced by a
**DMM-managed ASI plugin** using the current runtime mount-stat path.

This version uses the same validated 2.03.00 mount-stat path as the All Stats variant,
but only modifies Speed / SubIndex 15.

The plugin is not hardcoded to Rokade or Character ID 31378.

## Installation

Install the release ZIP through DMM and enable it as usual.

Do not enable this mod together with **All Mounts LvL 5 All Stats**.

## Compatibility guard

The 2.03.00 build contains a strict game-code signature check. If the expected
Crimson Desert 2.03.00 code does not match, the plugin fails closed instead of
blindly patching an unknown game build.
