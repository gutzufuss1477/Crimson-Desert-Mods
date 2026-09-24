# Mining Helmet Always On

Current mod version: **1.0.0**<br>
Supported game version: **Crimson Desert 2.03.00**

Automatically keeps the Mining Helmet's native blue mining-resource highlight active while any headgear is equipped. No Mining Helmet and no key press are required.

## Features

- Activates automatically after loading into the game.
- Shows the native blue highlight on nearby supported ore, mine and rock nodes.
- Works with any headgear.
- Does not enable the green full-screen Mining Helmet filter.
- Preserves the original Mining Helmet and its `B` key behavior while that helmet is equipped.
- Uses the game's normal native visibility and fade distance.

## Requirements

- Crimson Desert **2.03.00**
- An ASI loader, such as Ultimate ASI Loader, or a DMM setup that supports ASI plugins

The plugin contains a strict executable hash guard. On an unsupported game executable it fails closed instead of applying hooks to unknown code.

## Installation

### DMM

Import `Mining_Helmet_Always_On_2.03.00.zip`, enable the mod and apply it.

### Manual ASI installation

Install an ASI loader in the game's `bin64` folder, then copy `Mining_Helmet_Always_On_2.03.00.asi` from the ZIP into that folder.

Typical Steam path:

`Steam\steamapps\common\Crimson Desert\bin64`

## Controls

The mod starts automatically; no key is required.

- `F7`: write the current status file
- `F8`: enable or retry for the current session
- `F9`: disable for the current session

Status and diagnostic files are written to:

`%LOCALAPPDATA%\MiningHelmetAlwaysOn\MH125`

## Uninstallation

Disable/remove the mod in DMM, or delete `Mining_Helmet_Always_On_2.03.00.asi` from the game's `bin64` folder.

## Notes

- The glow range is the game's normal Mining Helmet range and was deliberately left unchanged.
- Mods that replace or force the same special-vision mode may conflict.
- No original game files are distributed or replaced.

## Source and third-party component

Source code is in `src/`. The plugin uses MinHook under its BSD 2-Clause license; the full license is included in the source tree and release package.
