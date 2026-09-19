# All Mounts LvL 5 All Stats

Current version: **2.03.00**

Sets the four compatible mount rank stats to Level 5:

- Movement Speed
- Acceleration
- Turning / Handling
- Jump

## 2.03.00 implementation

Crimson Desert 2.03.00 changed how active mount stats are resolved for existing mounts.
The previous static CharacterInfo data patch can still modify the game data, but those
values are no longer reliably used for the active stats of an already-owned Level 5 mount.

For 2.03.00 this mod therefore uses a **DMM-managed ASI plugin** and applies the values
through the current runtime mount-stat path.

The runtime path was validated in-game on an existing Level 5 Rokade:
**4 / 4 / 5 / 5 -> 5 / 5 / 5 / 5** without Cheat Engine.

The plugin is not hardcoded to Rokade or Character ID 31378. It targets the compatible
mount-stat rows identified by StatIndex 3 and SubIndex 15-18.

## Installation

Install the release ZIP through DMM and enable it as usual.

Do not enable this mod together with **All Mounts LvL 5 Speed**.

## Compatibility guard

The 2.03.00 build contains a strict game-code signature check. If the expected
Crimson Desert 2.03.00 code does not match, the plugin fails closed instead of
blindly patching an unknown game build.
